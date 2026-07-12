#include <debug.h>
#include <kernel.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <tamtypes.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 64

void initialize_iop()
{
  // int ret;

  // Initialize the Remote Procedure Call system to talk to the IOP
  SifInitRpc(0);

  // Since we will load the text file from the host, we don't need
  // to load any special modules in the IOP. However, it is always a
  // good idea to initialize the protocol anyway to avoid bugs.

  /*
  ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);

  if (ret < 0) {
    scr_printf("Failed to load SIO2MAN! (%d)", ret);
    SleepThread();
  }
  */
}

int main()
{
  // Variable to hold return values
  int ret;

  // Initialize the screen for debug
  init_scr();

  scr_printf("-- Loading file example in PS2 --\n");

  scr_printf("\nInitializing IOP...\n");
  initialize_iop();
  scr_printf("IOP initialized successfully!\n");

  FILE *fp = fopen("host:message.txt", "r");
  if (fp == NULL)
  {
    scr_printf("Failed to open file!\n");

    SleepThread();

    return 0;
  }

  scr_printf("\nFile opened successfully!\n");

  // Reading and displaying file contents
  scr_printf("\nFile's contents:\n\n");

  char buffer[BUFFER_SIZE];
  size_t count;

  while ((count = fread(buffer, sizeof(char), BUFFER_SIZE - 1, fp)) > 0)
  {
    buffer[count] = '\0';
    scr_printf("%s", buffer);
  }

  if (ferror(fp))
  {
    scr_printf("\n! An error ocurred while reading the file.");
  }
  else
  {
    scr_printf("\n\n-- End of file --\n");
  }
  // -------
  
  ret = fclose(fp);
  if (ret == 0)
  {
    scr_printf("File closed successfully!\n");
  }
  else
  {
    scr_printf("! Error closing file.\n");
  }

  scr_printf("\n-- End of EE program --\n");

  SleepThread();

  return 0;
}
