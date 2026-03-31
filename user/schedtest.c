#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
// Toggle cases:
// 1 = Case 1 (Increasing bursts): child1 < child2 < ... < child5
// 0 = Case 2 (Decreasing bursts): child1 > child2 > ... > child5
#define INCREASING 1

// Dummy calculations to simulate a CPU burst.
// lab says "burst time is simulated by loop iterations doing dummy calculations".
static void
cpu_burst(int iterations)
{
  // CPU BURST TIME
  // Simualted using looop iteration
  // therefore longer loops = longer CPU bursts
  volatile int x = 0;
  for (int i = 0; i < iterations; i++) {
    // dummy math to keep the loop from being optimized away
    x = x + (i % 7);
  }
}

// Simulated I/O burst: moves process out of RUNNING for a bit.
static void
io_burst(void)
{
  pause(0); // simulates I/O by yielding the CPU, letting other processes run
}

static void
child_process(int child_id)
{
  // child_id is 1..5
  // Choose iteration counts big enough to create clear differences in waiting_tick
  
  // LAB 4
  // CPU bursts depend on child number using arrays:
  int inc[5] = { 20000000, 40000000, 60000000, 80000000, 100000000 };
  int dec[5] = { 100000000, 80000000, 60000000, 40000000, 20000000 };
  // Case 1: increasing               Case 2: decreasing
  // Child 1 --> shortest burst       Child 1 --> longest burst
  // Child 5 --> longest burst        Child 2 --> shortest burst
  // lower avg waiting time           higher avg waiting time
  int iterations = INCREASING ? inc[child_id - 1] : dec[child_id - 1];
  // flips the cases here by flipping the compile time flag 
  // 3 rounds: CPU burst + I/O burst (matches lab)
  for (int j = 0; j < 3; j++) {
    cpu_burst(iterations);
    io_burst();
  }
}

int
main(void)
{
  // LAB 4
  // Each child represents a process w/ a different CPU burst length
  // Fork 5 children
  for (int i = 0; i < 5; i++) {
    int pid = fork();

    if (pid < 0) {
      printf("Fork failed for child %d\n", i + 1);
      exit(1);
    }

    if (pid == 0) {
      // child
      child_process(i + 1);
      exit(0);
    } else {
      // parent
      printf("Parent: Forked child %d with PID %d\n", i + 1, pid);
    }
  }

  // Parent waits for all children
  for (int i = 0; i < 5; i++) {
    wait(0);
  }

  exit(0);
}
