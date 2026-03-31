#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/eco.h"
#include "user/user.h"

// Converts the current scheduler mode to readable text
char*
mode_name(int mode)
{
  if(mode == ECO)
    return "ECO";
  if(mode == BALANCED)
    return "BALANCED";
  if(mode == PERF)
    return "PERF";
  return "UNKNOWN";
}

int
main(int argc, char *argv[])
{
  struct idlestats stats;
  int mode;

  // get idle CPU stats from the kernel
  if(getidlestats(&stats) < 0){
    fprintf(2, "eco_report: getidlestats failed\n");
    exit(1);
  }

  // get current scheduler mode from Person 3's syscall
  mode = getschedmode();

  printf("Eco Report\n");
  printf("----------\n");
  printf("mode: %s\n", mode_name(mode));
  printf("idle entries: %d\n", stats.idle_entries);
  printf("total idle ticks: %d\n", stats.total_idle_ticks);
  printf("currently idle cpus: %d\n", stats.currently_idle_cpus);

  exit(0);
}