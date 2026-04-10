#ifndef ECO_H
#define ECO_H

#include "types.h"

struct idlestats {
  uint idle_entries;
  uint total_idle_ticks;
  uint currently_idle_cpus;
};

#endif