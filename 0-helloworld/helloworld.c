#include <debug.h>
#include <kernel.h>

int main(void)
{
  // Initializes the screen so we can print text
  init_scr();

  // Prints our text!
  scr_printf("Hello, PS2 World!\n");

  // Put the EE to sleep (so our program doesn't immediately 'closes')
  SleepThread();
  return 0;
}
