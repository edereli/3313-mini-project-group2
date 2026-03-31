#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define PIPESIZE 32


// Bad pipe because it loses data and doesn't preserve order (never stops writing)
// When buffer fills up, it overwrites unread data instead of waiting

// This struct mimics xv6's core pipe idea in user space:
// - data[] stores bytes
// - nread/nwrite are counters (not indexes) that keep increasing
// - we use % PIPESIZE to wrap around the 32-byte array (circular buffer)
struct bad_pipe { // This is my pipe (has a 32-byte array data[32] where bytes are stored + 2 counters nread and nwrite)
  char data[PIPESIZE];
  uint nread;   // number of bytes read so far (total)
  uint nwrite;  // number of bytes written so far (total)
};

// Write ONE character into the circular buffer.
// Core pipe logic pattern (same idea as kernel pipe.c):
// write at data[nwrite % PIPESIZE], then increment nwrite.

// WRITE CHAR: I put it into the array at data[nwrite % 32]
// The % 32 wraps around so it becomes a circular buffer 
// Then I increment nwrite 
void
pipe_write(struct bad_pipe *pi, char ch)
{
  // Put the char into the buffer, wrapping around using modulo.
  pi->data[pi->nwrite % PIPESIZE] = ch;

  // Move the write counter forward (total bytes written increases).
  pi->nwrite++;
}

// Read ONE character from the circular buffer.
// If empty (nread == nwrite), return -1.
// Otherwise read data[nread % PIPESIZE], then increment nread.

//READ: check if empty (if nread == nwrite), if nothing left return -1
// Else: read data[nread % 32] --> increment nread --> return char
int
pipe_read(struct bad_pipe *pi)
{
  // Empty condition: nothing left to read.
  if (pi->nread == pi->nwrite)
    return -1;

  // Read the next byte (wrap around using modulo).
  int ch = (unsigned char)pi->data[pi->nread % PIPESIZE];

  // Move the read counter forward.
  pi->nread++;

  return ch;
}

// MAIN: reads from console one char at a time using read(0, &ch, 1) 
// I track last 3 chars in last3[]
// Everytime I read a new char I shift last3 and check if it form "o, k, ?"
// STOPPING LOGIC: when i detect ok? I stop reading and print
// After breaking I print what's stored by calling pipe_read() until it returns -1 (empty)
int
main(void)
{
  struct bad_pipe pipe;

  // Initialize counters (important so the pipe starts "empty")
  pipe.nread = 0;
  pipe.nwrite = 0;

  char ch;

  // Track last 3 typed characters so we can detect "ok?"
  char last3[3] = {0, 0, 0};

  printf("Type text. Enter 'ok?' to stop and display buffer contents.\n\n");

  // Read from stdin one character at a time
  while (read(0, &ch, 1) == 1) {

    // Update the sliding window of the last 3 characters typed
    last3[0] = last3[1];
    last3[1] = last3[2];
    last3[2] = ch;

    // Stop condition: if user typed "ok?"
    // Important: the sequence "ok?" should NOT appear in stored output.
    if (last3[0] == 'o' && last3[1] == 'k' && last3[2] == '?') {

      // 'o' and 'k' were already written on previous loop iterations.
      // We "undo" those last 2 writes by reducing nwrite by 2.
      // '?' was never written (we check before writing), so it won't appear either.
      if (pipe.nwrite >= 2)
        pipe.nwrite -= 2;

      // Now stop reading input
      break;
    }

    // Normal case: store the typed character into our bad pipe buffer
    pipe_write(&pipe, ch);
  }

  printf("\n\n--- Stored output (bad pipe buffer) ---\n");

  // Read back and print everything currently stored in the buffer
  // until the pipe becomes empty (pipe_read returns -1).
  int out;
  while ((out = pipe_read(&pipe)) != -1) {
    char c = (char)out;
    write(1, &c, 1);
  }

  printf("\n");
  exit(0);
}
