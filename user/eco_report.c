#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/eco.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct idlestats stats;

  if(getidlestats(&stats) < 0){
    fprintf(2, "eco_report: getidlestats failed\n");
    exit(1);
  }

  printf("Idle CPU stats\n");
  printf("--------------\n");
  printf("idle entries: %d\n", stats.idle_entries);
  printf("total idle ticks: %d\n", stats.total_idle_ticks);
  printf("currently idle cpus: %d\n", stats.currently_idle_cpus);

  exit(0);
}