#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "uproc.h"

#ifdef CS333_P3P4
struct state_lists {
  struct proc* ready[MAXPRIO + 1];
  struct proc* ready_tail[MAXPRIO + 1];
  struct proc* free;
  struct proc* free_tail;
  struct proc* sleep;
  struct proc* sleep_tail;
  struct proc* zombie;
  struct proc* zombie_tail;
  struct proc* running;
  struct proc* running_tail;
  struct proc* embryo;
  struct proc* embryo_tail;
};
#endif

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3P4
  struct state_lists pLists;
  uint PromoteAtTime;
#endif
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

#ifdef CS333_P3P4
static void initProcessLists(void);
static void initFreeList(void);
// __attribute__ ((unused)) suppresses warnings for routines that are not
// currently used but will be used later in the project. This is a necessary
// side-effect of using -Werror in the Makefile.
static void __attribute__ ((unused)) stateListAdd(struct proc** head, struct proc** tail, struct proc* p);
static void __attribute__ ((unused)) stateListAddAtHead(struct proc** head, struct proc** tail, struct proc* p);
static int __attribute__ ((unused)) stateListRemove(struct proc** head, struct proc** tail, struct proc* p);
static void assertState(struct proc* p, enum procstate state);
static void searchAbandonedChildren();
static void promoteProcs();
static void demoteProc();
#endif

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

#ifdef CS333_P3P4
  acquire(&ptable.lock);
  if (ptable.pLists.free == 0) {
    release(&ptable.lock);
    return 0;
  }
  else {
    p = ptable.pLists.free;
    int rc = stateListRemove(&ptable.pLists.free, &ptable.pLists.free_tail, ptable.pLists.free);
    if (rc == -1) {
      panic("failed list removal in allocproc\n");
    }
    assertState(p, UNUSED);
    goto found;
  }

#else
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;
#endif


found:
  p->state = EMBRYO;
#ifdef CS333_P3P4
  stateListAdd(&ptable.pLists.embryo, &ptable.pLists.embryo_tail, p);
#endif
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
#ifdef CS333_P3P4
    acquire(&ptable.lock);
    int rc = stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryo_tail, p);
    if (rc == -1) {
      panic("failed list removal in allocproc embryo to unused\n");
    }
    assertState(p, EMBRYO);
    p->state = UNUSED;
    stateListAdd(&ptable.pLists.free, &ptable.pLists.free_tail, p);
    release(&ptable.lock);
#else
    p->state = UNUSED;
#endif
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
#ifdef CS333_P1
  p->start_ticks = ticks;
#endif
#ifdef CS333_P2
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in = 0;
  p->elapsed = 0;
#endif
#ifdef CS333_P3P4
  p->priority = MAXPRIO;
  p->budget = BUDGET;
#endif
  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

#ifdef CS333_P3P4
  acquire(&ptable.lock);
  initProcessLists();
  initFreeList();
  release(&ptable.lock);
#endif
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  // set default gid and uid
#ifdef CS333_P2
  p->uid = DEFAULTUID;
  p->gid = DEFAULTGID;
#endif
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

#ifdef CS333_P3P4
  acquire(&ptable.lock);
  stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryo_tail, p);
  assertState(p, EMBRYO);
  p->state = RUNNABLE;
  p->next = 0;
  stateListAdd(&ptable.pLists.ready[MAXPRIO], &ptable.pLists.ready_tail[MAXPRIO], p);
  release(&ptable.lock);
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
#else
  p->state = RUNNABLE;
#endif
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
#ifdef CS333_P3P4
    acquire(&ptable.lock);
    int rc = stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryo_tail, ptable.pLists.embryo);
    if (rc == -1) {
      panic("failed removal in fork\n");
    }
    assertState(np, EMBRYO);
    np->state = UNUSED;
    stateListAdd(&ptable.pLists.free, &ptable.pLists.free_tail, np);
    release(&ptable.lock);
#else
    np->state = UNUSED;
#endif
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
  // copy UID and GID to new process
#ifdef CS333_P2
  np->uid = proc->uid;
  np->gid = proc->gid;
#endif

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
#ifdef CS333_P3P4
  int rc = stateListRemove(&ptable.pLists.embryo, &ptable.pLists.embryo_tail, ptable.pLists.embryo);
  if (rc == -1) {
    panic("failed removal in fork embryo to ready\n");
  }
  assertState(np, EMBRYO);
  np->state = RUNNABLE;
  np->priority = MAXPRIO;
  np->budget = BUDGET;
  stateListAdd(&ptable.pLists.ready[MAXPRIO], &ptable.pLists.ready_tail[MAXPRIO], np);
#else
  np->state = RUNNABLE;
#endif
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifndef CS333_P3P4
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#else
void
exit(void)
{
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  searchAbandonedChildren();


  // Jump into the scheduler, never to return.
  int rc = stateListRemove(&ptable.pLists.running, &ptable.pLists.running_tail, proc);
  if (rc == -1) {
    panic("failed removal in exit\n");
  }
  assertState(proc, RUNNING);
  proc->state = ZOMBIE;
  stateListAdd(&ptable.pLists.zombie, &ptable.pLists.zombie_tail, proc);
  sched();
  panic("zombie exit");
}
#endif

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifndef CS333_P3P4
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
wait(void)
{
  struct proc *p, *curr;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    havekids = 0;
    // search running list for child
    curr = ptable.pLists.running;
    while (curr && havekids == 0) {
      if (curr->parent == proc) {
        havekids = 1;
        p = curr;
      }
      curr = curr->next;
    }
    // search ready lists for child
    for (int i = 0; i < MAXPRIO + 1; i++) {
      curr = ptable.pLists.ready[i];
      while (curr && havekids == 0) {
        if (curr->parent == proc) {
          havekids = 1;
          p = curr;
        }
        curr = curr->next;
      }
    }
    curr = ptable.pLists.sleep;
    // search sleeping list for child
    while (curr && havekids == 0) {
      if (curr->parent == proc) {
        havekids = 1;
        p = curr;
      }
      curr = curr->next;
    }
    curr = ptable.pLists.zombie;
    // search zombie list for child
    while (curr && havekids == 0) {
      if (curr->parent == proc) {
        havekids = 1;
        p = curr;
      }
      curr = curr->next;
    }

    // zombie child
    if(p->state == ZOMBIE){
      pid = p->pid;
      kfree(p->kstack);
      p->kstack = 0;
      freevm(p->pgdir);
      // no need to release lock after removal and updating list - will be released elsewhere
      int rc = stateListRemove(&ptable.pLists.zombie, &ptable.pLists.zombie_tail, p);
      if (rc == -1) {
        panic("failed removal in wait helper\n");
      }
      assertState(p, ZOMBIE);
      p->state = UNUSED;
      stateListAdd(&ptable.pLists.free, &ptable.pLists.free_tail, p);
      p->pid = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;
      release(&ptable.lock);
      return pid;
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
    }
}
#endif

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifndef CS333_P3P4
// original xv6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
#ifdef CS333_P2
      p->cpu_ticks_in = ticks;
#endif
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}

#else
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    acquire(&ptable.lock);
    // promote procs if it's time
    if (ticks >= ptable.PromoteAtTime) {
      promoteProcs();
    }
    // search for a process to run starting with highest priority and going down if a proc not found
    for (int i = MAXPRIO; i >= 0; i--) {
      p = ptable.pLists.ready[i];
      if (p) {
        int rc = stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.ready_tail[i], p);
        if (rc == -1) {
          panic("failed removal in scheduler\n");
        }
        assertState(p, RUNNABLE);
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        idle = 0;  // not idle this timeslice
        proc = p;
        switchuvm(p);
        p->cpu_ticks_in = ticks;
        p->state = RUNNING;
        stateListAdd(&ptable.pLists.running, &ptable.pLists.running_tail, p);
        swtch(&cpu->scheduler, proc->context);
        switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
        proc = 0;
        break;
      }
    }
    release(&ptable.lock);
  }

  // if idle, wait for next interrupt
  if (idle) {
    sti();
    hlt();
  }
}
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
#ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
#endif
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
#ifdef CS333_P3P4
  demoteProc(proc);
  int rc = stateListRemove(&ptable.pLists.running, &ptable.pLists.running_tail, proc);
  if (rc == -1) {
    panic("failed removal in yield\n");
  }
  assertState(proc, RUNNING);
  proc->state = RUNNABLE;
  stateListAdd(&ptable.pLists.ready[proc->priority], &ptable.pLists.ready_tail[proc->priority], proc);
#else
  proc->state = RUNNABLE;
#endif
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
#ifdef CS333_P3P4
  // lock is acquired before this section and released after routine calls sched
  demoteProc(proc);
  int rc = stateListRemove(&ptable.pLists.running, &ptable.pLists.running_tail, proc);
  if (rc == -1) {
    panic("failed removal in sleep\n");
  }
  assertState(proc, RUNNING);
  proc->state = SLEEPING;
  stateListAdd(&ptable.pLists.sleep, &ptable.pLists.sleep_tail, proc);
#else
  proc->state = SLEEPING;
#endif
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

#ifndef CS333_P3P4
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#else
static void
wakeup1(void *chan)
{
  struct proc *current = ptable.pLists.sleep;
  struct proc *p;

  while (current) {
    p = current;
    current = current->next;
    if (p->chan == chan) {
      int rc = stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleep_tail, p);
      if (rc == -1) {
        panic("failed removal in wakeup\n");
      }
      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.pLists.ready[p->priority], &ptable.pLists.ready_tail[p->priority], p);
    }
  }

}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifndef CS333_P3P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#else
int
kill(int pid)
{
  struct proc *curr, *p;
  int found = 0;
  acquire(&ptable.lock);
  curr = ptable.pLists.running;
  while (curr && found == 0) {
    if (curr->pid == pid) {
      p = curr;
      found = 1;
    }
      curr = curr->next;
  }
  for (int i = MAXPRIO; i >= 0; i--) {
    curr = ptable.pLists.ready[i];
    while (curr && found == 0) {
      if (curr->pid == pid) {
        p = curr;
        found = 1;
      }
        curr = curr->next;
    }
  }
  curr = ptable.pLists.sleep;
  while (curr && found == 0) {
    if (curr->pid == pid) {
      p = curr;
      found = 1;
    }
      curr = curr->next;
  }
  curr = ptable.pLists.zombie;
  while (curr && found == 0) {
    if (curr->pid == pid) {
      p = curr;
      found = 1;
    }
      curr = curr->next;
  }
  curr = ptable.pLists.free;
  while (curr && found == 0) {
    if (curr->pid == pid) {
      p = curr;
      found = 1;
    }
      curr = curr->next;
  }
  curr = ptable.pLists.embryo;
  while (curr && found == 0) {
    if (curr->pid == pid) {
      p = curr;
      found = 1;
    }
      curr = curr->next;
  }

  if (found == 1) {
    p->killed = 1;
    // Wake process from sleep if necessary.
    if(p->state == SLEEPING) {
      int rc = stateListRemove(&ptable.pLists.sleep, &ptable.pLists.sleep_tail, p);
      if (rc == -1) {
        panic("failed removal of sleeping proc in kill - likely proc was not in sleep list but had sleep state\n");
      }
      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.pLists.ready[MAXPRIO], &ptable.pLists.ready_tail[MAXPRIO], p);
    }
    release(&ptable.lock);
    return 0;
  }
  release(&ptable.lock);
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.

void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

#ifdef CS333_P3P4
  int elapsedAfterDec, cpuAfterDec, ppid;
  int elapsed;
  cprintf("\n");
  cprintf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s", "PID", "Name", "UID", "GID", "PPID", "Prio", "Elapsed", "CPU", "State", "Size", "PCs");
  cprintf("\n");
#elif CS333_P2
  int elapsedAfterDec, cpuAfterDec, ppid;
  int elapsed;
  cprintf("\n");
  cprintf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s", "PID", "Name", "UID", "GID", "PPID", "Elapsed", "CPU", "State", "Size", "PCs");
  cprintf("\n");
#elif CS333_P1
  int elapsed;
  cprintf("\n");
  cprintf("%s\t%s\t%s\t%s\t%s", "PID", "State", "Name", "Elapsed", "PCs");
  cprintf("\n");
#endif
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
#ifdef CS333_P3P4
    elapsed = ticks - p->start_ticks;
    elapsedAfterDec = elapsed % 1000;
    cpuAfterDec = p->cpu_ticks_total % 1000;
    if (!p->parent) {
      ppid = p->pid;
    }
    else {
      ppid = p->parent->pid;
    }
    cprintf("%d\t%s\t%d\t%d\t%d\t%d\t%d.%d\t%d.%d\t%s\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, p->priority, elapsed / 1000, elapsedAfterDec, p->cpu_ticks_total / 1000, cpuAfterDec, state, p->sz);
#elif CS333_P2
    elapsed = ticks - p->start_ticks;
    elapsedAfterDec = elapsed % 1000;
    cpuAfterDec = p->cpu_ticks_total % 1000;
    if (!p->parent) {
      ppid = p->pid;
    }
    else {
      ppid = p->parent->pid;
    }
    cprintf("%d\t%s\t%d\t%d\t%d\t%d.%d\t%d.%d\t%s\t%d\t", p->pid, p->name, p->uid, p->gid, ppid, elapsed / 1000, elapsedAfterDec, p->cpu_ticks_total / 1000, cpuAfterDec, state, p->sz);
#elif CS333_P1
    elapsed = ticks - p->start_ticks;
    cprintf("%d\t%s\t%s\t%d.%d\t", p->pid, state, p->name, elapsed / 1000, elapsed % 1000);
#else
    cprintf("%d %s %s", p->pid, state, p->name);
#endif
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf("%p ", pc[i]);
    }
    cprintf("\n");
  }
}

#ifdef CS333_P3P4
static void
stateListAdd(struct proc** head, struct proc** tail, struct proc* p)
{
  if(*head == 0){
    *head = p;
    *tail = p;
    p->next = 0;
  } else {
    (*tail)->next = p;
    *tail = (*tail)->next;
    (*tail)->next = 0;
  }
}

static void
stateListAddAtHead(struct proc** head, struct proc** tail, struct proc* p)
{
  if(*head == 0){
    *head = p;
    *tail = p;
    p->next = 0;
  } else {
    p->next = *head;
    *head = p;
  }
}

static int
stateListRemove(struct proc** head, struct proc** tail, struct proc* p)
{
  if(*head == 0 || *tail == 0 || p == 0){
    return -1;
  }

  struct proc* current = *head;
  struct proc* previous = 0;

  if(current == p){
    *head = (*head)->next;
    // prevent tail remaining assigned when we've removed the only item
    // on the list
    if(*tail == p){
      *tail = 0;
    }
    return 0;
  }

  while(current){
    if(current == p){
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found, hit eject.
  if(current == 0){
    return -1;
  }

  // Process found. Set the appropriate next pointer.
  if(current == *tail){
    *tail = previous;
    (*tail)->next = 0;
  } else {
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = 0;

  return 0;
}

static void
initProcessLists(void)
{
  for (int i = 0; i < MAXPRIO + 1; i++) {
    ptable.pLists.ready[i] = 0;
    ptable.pLists.ready_tail[i] = 0;
  }
  ptable.pLists.free = 0;
  ptable.pLists.free_tail = 0;
  ptable.pLists.sleep = 0;
  ptable.pLists.sleep_tail = 0;
  ptable.pLists.zombie = 0;
  ptable.pLists.zombie_tail = 0;
  ptable.pLists.running = 0;
  ptable.pLists.running_tail = 0;
  ptable.pLists.embryo = 0;
  ptable.pLists.embryo_tail = 0;
}

static void
initFreeList(void)
{
  if(!holding(&ptable.lock)){
    panic("acquire the ptable lock before calling initFreeList\n");
  }

  struct proc* p;

  for(p = ptable.proc; p < ptable.proc + NPROC; ++p){
    p->state = UNUSED;
    stateListAdd(&ptable.pLists.free, &ptable.pLists.free_tail, p);
  }
}

#endif

// Pulls all processes in the systems from ptable and stores into uproc struct
#ifdef CS333_P2
int
uprocHelper(uint max, struct uproc* uprocs)
{
  uint strLength = 0, numProcs = 0;
  for (int i = 0; i < max; i++) {
    if (ptable.proc[i].state == RUNNING || ptable.proc[i].state == RUNNABLE || ptable.proc[i].state == SLEEPING || ptable.proc[i].state == ZOMBIE) {
      uprocs[i].pid = ptable.proc[i].pid;
      uprocs[i].uid = ptable.proc[i].uid;
      uprocs[i].gid = ptable.proc[i].gid;
      if (!ptable.proc[i].parent) {
        uprocs[i].ppid = ptable.proc[i].pid;
      }
      else {
        uprocs[i].ppid = ptable.proc[i].parent->pid;
      }
#ifdef CS333_P3P4
      uprocs[i].priority = ptable.proc[i].priority;
#endif
      uprocs[i].elapsed_ticks = ticks - ptable.proc[i].start_ticks;
      uprocs[i].CPU_total_ticks = ptable.proc[i].cpu_ticks_total;
      if (ptable.proc[i].state == RUNNING) {
        safestrcpy(uprocs[i].state, "run", 4);
      }
      else if (ptable.proc[i].state == SLEEPING) {
        safestrcpy(uprocs[i].state, "sleep", 6);
      }
      else if (ptable.proc[i].state == RUNNABLE) {
        safestrcpy(uprocs[i].state, "runnable", 9);
      }
      else if (ptable.proc[i].state == ZOMBIE) {
        safestrcpy(uprocs[i].state, "zomb", 5);
      }
      uprocs[i].size = ptable.proc[i].sz;
      strLength = strlen(ptable.proc[i].name);
      safestrcpy(uprocs[i].name, ptable.proc[i].name, strLength + 1);
      numProcs++;
      // if hit max number of procs in xv6 return
      if (numProcs == 64) {
        return numProcs;
      }
    }
  }
  return numProcs;
}
#endif

#ifdef CS333_P3P4
// Prints the required control console commands based on the state passed in
void
plistHelper(enum procstate state)
{
  int n = 0;
  struct proc *curr;
  acquire(&ptable.lock);
  if (state == SLEEPING) {
    curr = ptable.pLists.sleep;
    cprintf("Sleep List Processes:\n");
    while (curr != 0) {
      if (curr->next == 0) {
        cprintf("%d\n", curr->pid);
        curr = curr->next;
      }
      else {
        cprintf("%d->", curr->pid);
        curr = curr->next;
      }
    }
  }
  else if (state == RUNNABLE) {
    cprintf("Ready List Processes\n");
    for (int i = MAXPRIO; i >= 0; i--) {
      cprintf("PRIORITY QUEUE %d: ", i);
      curr = ptable.pLists.ready[i];
      while (curr != 0) {
        if (curr->next == 0) {
          cprintf("(P: %d, B: %d)", curr->pid, curr->budget);
          curr = curr->next;
        }
        else {
          cprintf("(P: %d, B: %d)->", curr->pid, curr->budget);
          curr = curr->next;
        }
      }
      cprintf("\n\n");
    }
  }
  else if (state == UNUSED) {
    curr = ptable.pLists.free;
    while (curr != 0) {
      n++;
      curr = curr->next;
    }
    cprintf("Free List Size: %d processes\n", n);
  }
  else if (state == ZOMBIE) {
    curr = ptable.pLists.zombie;
    cprintf("Zombie List Processes:\n");
    while (curr != 0) {
      if (curr->next != 0) {
        cprintf("(%d,%d)->", curr->pid, curr->parent->pid);
        curr = curr->next;
      }
      else {
        cprintf("(%d, %d)\n", curr->pid, curr->parent->pid);
        curr = curr->next;
      }
    }
  }
  release(&ptable.lock);

}

// Asserts proc state is correct before adding to a list
static void
assertState(struct proc* p, enum procstate state)
{
  if (p->state != state) {
    panic("proc state not correct when performing state transition\n");
  }
}

// Search running, ready, sleep and zombie lists for abandoned children
static void
searchAbandonedChildren()
{
  int finished = 0;
  struct proc* curr;
  while (finished != 1) {
    // searches running list
    curr = ptable.pLists.running;
    while (curr) {
      if (curr->parent == proc) {
        curr->parent = initproc;
        if (curr->state == ZOMBIE) {
          wakeup1(initproc);
        }
      }
      curr = curr->next;
    }
    // search ready list
    for (int i = 0; i < MAXPRIO + 1; i++) {
      curr = ptable.pLists.ready[i];
      while (curr) {
        if (curr->parent == proc) {
          curr->parent = initproc;
          if (curr->state == ZOMBIE) {
            wakeup1(initproc);
          }
        }
        curr = curr->next;
      }
    }
    // search sleep list
    curr = ptable.pLists.sleep;
    while (curr) {
      if (curr->parent == proc) {
        curr->parent = initproc;
        if (curr->state == ZOMBIE) {
          wakeup1(initproc);
        }
      }
      curr = curr->next;
    }
    // sleep zombie list
    curr = ptable.pLists.zombie;
    while (curr) {
      if (curr->parent == proc) {
        curr->parent = initproc;
        if (curr->state == ZOMBIE) {
          wakeup1(initproc);
        }
      }
      curr = curr->next;
    }
    finished = 1;
  }
}

// Search active lists for pid match to set priority
int
setPriorityHelper(int pid, int priority)
{
  struct proc* curr;

  // search running list
  acquire(&ptable.lock);
  curr = ptable.pLists.running;
  while (curr) {
    if (curr->pid == pid) {
      curr->priority = priority;
      curr->budget = BUDGET;
      release(&ptable.lock);
      return 0;
    }
    curr = curr->next;
  }
  // search sleep list
  curr = ptable.pLists.sleep;
  while (curr) {
    if (curr->pid == pid) {
      curr->priority = priority;
      curr->budget = BUDGET;
      release(&ptable.lock);
      return 0;
    }
    curr = curr->next;
  }
  // search all ready lists
  for (int i = 0; i < MAXPRIO + 1; i++) {
    curr = ptable.pLists.ready[i];
    while (curr) {
      if (curr->pid == pid) {
        // don't move proc if its priority is the same
        if (curr->priority == priority) {
          release(&ptable.lock);
          return 0;
        }
        else {
          // atomically move proc to new ready list
          int rc = stateListRemove(&ptable.pLists.ready[i], &ptable.pLists.ready_tail[i], curr);
          if (rc == -1) {
            panic("failed removal in priority helper\n");
          }
          assertState(curr, RUNNABLE);
          if (curr->priority != MAXPRIO) {
            curr->priority = priority;
            curr->budget = BUDGET;
          }
          stateListAdd(&ptable.pLists.ready[priority], &ptable.pLists.ready_tail[priority], curr);
          release(&ptable.lock);
          return 0;
        }
      }
      curr = curr->next;
    }
  }
  // pid not found so return error
  release(&ptable.lock);
  return -1;

}

int
getPriorityHelper(int pid)
{
  struct proc* curr;

  // search running list
  curr = ptable.pLists.running;
  while (curr) {
    if (curr->pid == pid) {
      return curr->priority;
    }
    curr = curr->next;
  }
  // search sleep list
  curr = ptable.pLists.sleep;
  while (curr) {
    if (curr->pid == pid) {
      return curr->priority;
    }
    curr = curr->next;
  }
  // search all ready lists
  for (int i = 0; i < MAXPRIO + 1; i++) {
    curr = ptable.pLists.ready[i];
    if (curr->pid == pid) {
      return curr->priority;
    }
    curr = curr->next;
  }

  // pid not found so return error
  return -1;


}

// Promote processes one priority level
static void
promoteProcs()
{
  struct proc* curr;
  struct proc* next;
  if (MAXPRIO != 0) {
    for (int i = MAXPRIO; i > 0; i--) {
      curr = ptable.pLists.ready[i - 1];
      while (curr) {
        next = curr->next;
        stateListRemove(&ptable.pLists.ready[i - 1], &ptable.pLists.ready_tail[i - 1], curr);
        assertState(curr, RUNNABLE);
        curr->priority = i;
        curr->budget = BUDGET;
        stateListAdd(&ptable.pLists.ready[i], &ptable.pLists.ready_tail[i], curr);
        curr = next;
      }
    }
    curr = ptable.pLists.running;
    while (curr) {
      if (curr->priority < MAXPRIO) {
        curr->priority++;
        curr->budget = BUDGET;
      }
      curr = curr->next;
    }
    curr = ptable.pLists.sleep;
    while (curr) {
      if (curr->priority < MAXPRIO) {
        curr->priority++;
        curr->budget = BUDGET;
      }
      curr = curr->next;
    }
    ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
  }
}

// Demote proc if budget is expired
static void
demoteProc(struct proc* p)
{
  if (MAXPRIO != 0) {
    p->budget = p->budget - (ticks - p->cpu_ticks_in);
    if (p->budget <= 0) {
      if (p->priority != 0) {
        p->priority = p->priority - 1;
        p->budget = BUDGET;
      }
    }
  }
}

#endif
