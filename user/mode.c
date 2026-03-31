#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static char *
modename(int mode)
{
  switch(mode){
  case ECO:
    return "ECO";
  case BALANCED:
    return "BALANCED";
  case PERF:
    return "PERF";
  default:
    return "UNKNOWN";
  }
}

int
main(int argc, char *argv[])
{
  int mode;

  if(argc == 1){
    mode = getschedmode();
    printf("current mode: %s\n", modename(mode));
    exit(0);
  }

  if(argc != 2){
    fprintf(2, "usage: mode [eco|balanced|perf]\n");
    exit(1);
  }

  if(strcmp(argv[1], "eco") == 0){
    mode = ECO;
  } else if(strcmp(argv[1], "balanced") == 0){
    mode = BALANCED;
  } else if(strcmp(argv[1], "perf") == 0){
    mode = PERF;
  } else {
    fprintf(2, "mode: invalid mode '%s'\n", argv[1]);
    fprintf(2, "usage: mode [eco|balanced|perf]\n");
    exit(1);
  }

  if(setschedmode(mode) < 0){
    fprintf(2, "mode: failed to set mode\n");
    exit(1);
  }

  printf("mode set to %s\n", modename(mode));
  exit(0);
}