// Creating the User Program with new file: user/ps.c
// This program accepts the command-line arguments, passes them to kps() system call
// and prints a usage message if no argument is provided
// This user program takes -o or -l from the command line and passes it to the kernel
// which calls the system call

#include "kernel/types.h" // including the basic xv6 data types
#include "user/user.h" // including user-lvl system call defs 

// User program entry point
int
main(int argc, char *argv[])
{
  // Checks that the user provided a command line argument 
  // Argument should be either "-o" or "-l"
  if(argc < 2){
    printf("Usage: ps [-o | -l]\n");
    exit(1);
  }

  // Calls the kps system call and passes the argument
  kps(argv[1]); // argc[1] determines whether to print short (-o) or long (-l) output
  exit(0); // exit the program successfully
}
