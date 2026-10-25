#include <debug.h>
#include <kernel.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
  char cwd[256];

  init_scr();
  scr_printf("argc = %d\n", argc);
  for (int i = 0; i < argc; i++)
    scr_printf("argv[%d] = %s\n", i, argv[i]);

  scr_printf("cwd = %s\n", getcwd(cwd, sizeof(cwd)) ? cwd : "(error)");

  FILE *f = fopen("img/sia.png", "rb");
  if (f) { scr_printf("fopen OK\n"); fclose(f); }
  else   { scr_printf("fopen failed: %d (%s)\n", errno, strerror(errno)); }

  SleepThread();
  return 0;
}