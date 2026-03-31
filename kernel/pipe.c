#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open

  // Adding Peterson algorthm fields 
  // volatile b/c values are shared btwn 2 processes, must not let complier cache or redorder them 
  // flag[0] = writer wants to enter (not interested)
  // flag[1] = reader wants to enter (i want to enter)
  volatile int flag[2]; // "Do I WANT to enter the critical section"
  
  // turn = 0 --> writer goes next
  // turn = 1 --> reader goes next
  volatile int turn; // "If we BOTH want in, whose turn is it?"

};

// Adding the Peterson helper functions:
// Peterson_Enter:
// me is 0 = writer, 1 = reader
static inline void
peterson_enter(struct pipe *pi, int me)
{
  int other = 1 - me; // identifies the other process ( writer(0) - other is reader (1) and vice versa)
  pi->flag[me] = 1; // You say "I want in"
  pi->turn = other; // If we both want to enter, lets the OTHER go 1st (prevents deadlock)

  __sync_synchronize(); // memmory fence (ensures correct ordering of shared memory writes)

  while(pi->flag[other] && pi->turn == other) // if the other process wants in AND it's their turn, I wait
    ; // spin
}

// Peterson_Exit: leaving the critical section
static inline void
peterson_exit(struct pipe *pi, int me)
{
  __sync_synchronize(); // memory fence (all read/writes in the critical section are complete)
  pi->flag[me] = 0; // "Im done the other process can enter now"
}

int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((pi = (struct pipe*)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;

  // LAB 5:
  // Peterson Alg initialization in pipealloc()
  pi->flag[0] = 0;
  pi->flag[1] = 0;
  pi->turn = 0;

  initlock(&pi->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;
  return 0;

 bad:
  if(pi)
    kfree((char*)pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

// LAB 5
// Runs when someone is DONE using the pipe 
// Marks one end of the pipe as closed
// --> if both ends are closed, it safely frees the pipe using Peterson's alg for mutual exclusion
void
pipeclose(struct pipe *pi, int writable)
{
  // Use Peterson to safely change the shared open flags.
  // If writable==1, we are closing the write end (writer = 0).
  // If writable==0, we are closing the read end  (reader = 1).
  int me = writable ? 0 : 1; /// who's closing?

  // Prevednts reader + writer from closing at the same time + messing things up
  peterson_enter(pi, me); // Enter Peterson - lock pipe safely 

  // Mark pipe end as closed
  if(writable){
    pi->writeopen = 0;
  } else {
    pi->readopen = 0;
  }

  // If BOTH ends are closed (is anyone still using this pipe?)
  int dofree = (pi->readopen == 0 && pi->writeopen == 0);
 
  peterson_exit(pi, me); // Leave Peterson (unlock)

  // Free's the pipe if no one needs it 
  if(dofree){
    kfree((char*)pi);
  }
}


// LAB 5 
// Peterson's alg replaces the sleep/wakeup (sleep requires a lock)
int
pipewrite(struct pipe *pi, uint64 addr, int n)
{
  int i = 0; // how many bytes we've written so far
  struct proc *pr = myproc(); // current process (the writer)
  char ch; // 1 byte to write at a time

  while(i < n){
    // same safety checks as before
    if(pi->readopen == 0 || killed(pr)){
      return -1;
    }

    // Enter Peterson critical section as WRITER (me = 0)
    // ensures only 1 side (reader or writer) touches pipe data
    // ensures Mutual Exclusion --> only writer OR reader can access the pipe buffer at 1 time
    peterson_enter(pi, 0); // these used to be locking (now replaced w/ peterson)


    // if pipe buffer is full, we can't write rn, exit CS and spin
    if(pi->nwrite == pi->nread + PIPESIZE){
      // When pipe = full
      // --> writer releases the critical section + spins until the reader consumes the data 
      peterson_exit(pi, 0); // leaves the critical section so the reader can run
      // ^ exits before spinning as the reader wouldn't be able to enter then and drain the pipe --> causing deadlock
      continue; // tries again later (spin)
    }

    // pipe has space (not full): 
    // copy 1 byte from user space -> write into pipe (kernel space)
    if(copyin(pr->pagetable, &ch, addr + i, 1) == -1){
      peterson_exit(pi, 0);
      break;
    }
    // write the byte into the pipe's circular buffer 
    pi->data[pi->nwrite % PIPESIZE] = ch;
    pi->nwrite++; // advance the write pointer 
    i++; // count 1 more byte written

    // leave critical section
    peterson_exit(pi, 0);
  }

  return i; // return # of bytes successfully written
}


// LAB 5
// Function lets the reader take bytes out of the pipe safetly, 1 at a time, while sharing the pipe w the writer
// Peterson's alg ensures mutual exclusion
// Reader + writer never touch the pipe buffer at the same time
// If pipe is empty, the read releases the critical section + waits
// This allows the writer to enter + add data 
int
piperead(struct pipe *pi, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();
  char ch;

  while(i < n){

    // if process got killed, stop (same behavior as original)
    if(killed(pr)){
      return -1;
    }

    // Enters Peterson (reader (me = 1))
    // enter Peterson critical section as READER (id = 1)
    peterson_enter(pi, 1);

    // if pipe is empty
    if(pi->nread == pi->nwrite){

      // Case A - if writer closed, then no more data will ever come (EOF)
      // return what I've read so far
      if(pi->writeopen == 0){
        peterson_exit(pi, 1);
        return i;    // return how many bytes we read so far
      }

      // Case B - writer still open, so data might arrive later
      // leave critical section so writer can run
      // since there's nothing to read yet, let go of pipe so the writer can add data 
      // we exit Peterson before waiting
      peterson_exit(pi, 1);
      continue;      // spin until data appears
    }

    // pipe has data: take one byte from circular buffer
    // grab 1 byte --> move the read pointer forward 
    ch = pi->data[pi->nread % PIPESIZE];
    pi->nread++;

    // leave critical section ASAP (leave Peterson so writer can run)
    peterson_exit(pi, 1);

    // copy that byte to the user buffer
    // moves byte from kernel memory --> user memory
    if(copyout(pr->pagetable, addr + i, &ch, 1) == -1){
      // match old behavior: if first byte fails, return -1
      if(i == 0)
        return -1;
      break;
    }

    i++; // repeats till n bytes or EOF
  }

  return i;
}