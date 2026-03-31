#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int p[2];
  if(pipe(p) < 0){
    fprintf(2, "pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if(pid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(pid == 0){
    // CHILD = reader
    close(p[1]); // close write end

    char buf[64];
    int n;

    while((n = read(p[0], buf, sizeof(buf))) > 0){
      write(1, buf, n);   // print to console
    }

    close(p[0]);
    exit(0);
  } else {
    // PARENT = writer
    close(p[0]); // close read end

    char *haiku =
      "An old silent pond\n"
      "A frog jumps into the pond—\n"
      "Splash! Silence again.\n";

    // Write the full string
    write(p[1], haiku, strlen(haiku));

    // Important: close write end so child sees EOF and stops
    close(p[1]);

    wait(0);
    exit(0);
  }
}
