#include <debug.h>
#include <kernel.h>
#include <libpad.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <stdio.h>
#include <tamtypes.h>
#include <unistd.h>
#include <graph.h>

// pad_dma_buf is provided by the user, one buf for each pad
// contains the pad's current state.
// libpad requires a strictly aligned memory buffer to receive data from the
// IOP.
static char padBuf[256] __attribute__((aligned(64)));

void load_iop_modules() {
  int ret;

  // Initialize the Remote Procedure Call system to talk to the IOP
  SifInitRpc(0);

  // Load the Serial I/O and Controller modules from the PS2's internal ROM

  /*
  SIO2MAN (Serial I/O 2 Manager): This is the low-level driver that
  knows how to manage the physical serial ports on the front of the
  console. It handles the raw electrical timing and bit-shifting.
  */
  ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);

  if (ret < 0) {
    scr_printf("Failed to load SIO2MAN! (%d)", ret);
    SleepThread();
  }

  /*
  PADMAN (Pad Manager): This driver sits on top of SIO2MAN. While
  SIO2MAN handles the raw bits, PADMAN knows how to translate those
  bits into the specific Sony PlayStation controller protocol
  (translating raw electrical pulses into "Triangle was pressed").
  */
  ret = SifLoadModule("rom0:PADMAN", 0, NULL);

  if (ret < 0) {
    scr_printf("Failed to load PADMAN! (%d)", ret);
    SleepThread();
  }
}

int main(int argc, char *argv[]) {
  init_scr();

  scr_printf("Hello, World!\n\n");
  scr_printf("Loading IOP modules...\n");

  load_iop_modules();

  scr_printf("Modules loaded!\n");
  scr_printf("Press a button\n");

  // Let's read controller inputs!
  int ret, old_state = -1;
  struct padButtonStatus buttons;
  u32 paddata, old_pad = 0, new_pad;

  ret = padInit(0);
  if (ret != 1) {
    scr_printf("padInit failed: %d\n", ret);
    SleepThread();
  }

  // Port 0 -> Connector 1 | port 1 -> Connector 2
  // Slot = 0 if not using multitap
  ret = padPortOpen(0, 0, padBuf);
  if (ret == 0) {
    scr_printf("padOpenPort failed: %d\n", ret);
    SleepThread();
  }

  while (1) {
    // Wait for screen refresh
    graph_wait_vsync();

    // Ask the IOP for the current status of the controller
    ret = padGetState(0, 0);

    // Only read if the controller is fully connected and stable
    while ((ret != PAD_STATE_STABLE) && (ret != PAD_STATE_FINDCTP1)) {
      if (ret != old_state) {
        if (ret == PAD_STATE_DISCONN) {
          scr_printf("Pad(%d, %d) is disconnected\n", 0, 0);
        }
      }
      old_state = ret;
      ret = padGetState(0, 0);
    }
    old_state = ret;

    // Read the data into our 'buttons' struct
    ret = padRead(0, 0, &buttons);

    if (ret == 0) {
      scr_printf("Failed to read pads: %d\n", ret);
      continue; // Try again
    }

    // Performing a not using xor to ensure bit consistencty
    // due to 16-bit to 32-bit conversion
    paddata = 0xFFFF ^ buttons.btns;
    new_pad = paddata & ~old_pad;
    old_pad = paddata;

    // Handle button presses
    if (new_pad & PAD_CROSS) {
      scr_printf("You pressed X :D\n");
    }

    if (new_pad & PAD_TRIANGLE) {
      scr_printf("You pressed TRIANGLE. Exiting...\n");
      break;
    }
  }
  // Wait for screen refresh
  graph_wait_vsync();
  scr_printf("\nGoing to mimir...\n");
  
  SleepThread();

  return 0;
}
