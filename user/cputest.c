#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
burn_cpu(void)
{
  volatile int x = 0;

  for(int i = 0; i < 50000000; i++){
    x += i % 7;
  }
}

int
main(void)
{
  int pid;
  int child_pid;
  int cpu_time;
  int pipefd[2];
  char done = 'x';

  if(pipe(pipefd) < 0){
    printf("cputest: pipe failed\n");
    exit(1);
  }

  pid = fork();
  if(pid < 0){
    printf("cputest: fork failed\n");
    close(pipefd[0]);
    close(pipefd[1]);
    exit(1);
  }

  if(pid == 0){
    close(pipefd[0]);
    burn_cpu();

    // Tell the parent the CPU-heavy work is done before exiting.
    write(pipefd[1], &done, 1);
    close(pipefd[1]);
    exit(0);
  }

  close(pipefd[1]);
  if(read(pipefd[0], &done, 1) != 1){
    printf("cputest: read failed\n");
    close(pipefd[0]);
    kill(pid);
    wait(0);
    exit(1);
  }
  close(pipefd[0]);

  // Record the child's CPU ticks before wait(); wait() reaps the child.
  child_pid = pid;
  cpu_time = getcputime(child_pid);
  if(wait(0) < 0){
    printf("cputest: wait failed\n");
    exit(1);
  }

  printf("child %d used %d CPU ticks\n", child_pid, cpu_time);
  exit(0);
}