#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NCHILD 3

// Burns CPU for a while so scheduler differences have time to show up
static void
burn_cpu(int rounds)
{
  volatile int x = 0;

  for(int r = 0; r < rounds; r++){
    for(int i = 0; i < 50000000; i++){
      x += i % 7;
    }
  }
}

// Small helper to sort results by pid before printing
static void
sort_results(int pids[], int ticks[], int n)
{
  int i, j, temp;

  for(i = 0; i < n; i++){
    for(j = i + 1; j < n; j++){
      if(pids[j] < pids[i]){
        temp = pids[i];
        pids[i] = pids[j];
        pids[j] = temp;

        temp = ticks[i];
        ticks[i] = ticks[j];
        ticks[j] = temp;
      }
    }
  }
}

int
main(void)
{
  int i;
  int pid;
  int pids[NCHILD];
  int ticks[NCHILD];
  int finished = 0;

  // Make 3 CPU-bound children with different workloads
  for(i = 0; i < NCHILD; i++){
    pid = fork();
    if(pid < 0){
      printf("cputest: fork failed\n");
      exit(1);
    }

    if(pid == 0){
      // Different children do different amounts of work
      burn_cpu(i + 1);
      exit(0);
    }

    pids[i] = pid;
    ticks[i] = -1;
  }

  // Give children some time to run before measuring
  pause(100);

  // Read cpu ticks while children still exist
  for(i = 0; i < NCHILD; i++){
    ticks[i] = getcputime(pids[i]);
  }

  // Reap all children
  while(finished < NCHILD){
    if(wait(0) < 0){
      printf("cputest: wait failed\n");
      exit(1);
    }
    finished++;
  }

  sort_results(pids, ticks, NCHILD);

  printf("cputest results:\n");
  for(i = 0; i < NCHILD; i++){
    printf("child %d used %d CPU ticks\n", pids[i], ticks[i]);
  }

  exit(0);
}