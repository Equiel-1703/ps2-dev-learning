#include <tamtypes.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <stdio.h>
#include <malloc.h>
#include <kernel.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

#define COLOR_BLACK   GS_SETREG_RGBAQ(0, 0, 0, 0, 0)
#define COLOR_PURPLE  GS_SETREG_RGBAQ(105, 15, 184, 0, 0)

#define SQUARE_SIZE   100.0f
#define SQUARE_SPEED  10.0f

typedef struct _square
{
  float x, y;
  float size;
  u64 color;
} Square;

static void draw_square(GSGLOBAL *gs, Square *sq)
{
  gsKit_prim_sprite(gs, 
    sq->x, sq->y,
    sq->x + sq->size, sq->y + sq->size,
    1,
    sq->color);
}

static void process_inputs_square(struct padButtonStatus *buttons, Square *sq)
{
  static int buttons_status;
  buttons_status = 0xFFFF ^ buttons->btns;

  if (buttons_status & PAD_UP)
  {
    sq->y -= SQUARE_SPEED;
  }
  if (buttons_status & PAD_DOWN)
  {
    sq->y += SQUARE_SPEED;
  }
  if (buttons_status & PAD_LEFT)
  {
    sq->x -= SQUARE_SPEED;
  }
  if (buttons_status & PAD_RIGHT)
  {
    sq->x += SQUARE_SPEED;
  }
}

static void init_dmac(void)
{
  // Initialize the DMA Controller.
  dmaKit_init(
      D_CTRL_RELE_OFF,
      D_CTRL_MFD_OFF,
      D_CTRL_STS_UNSPEC,
      D_CTRL_STD_OFF,
      D_CTRL_RCYC_8,
      1 << DMA_CHANNEL_GIF);
  // Initialize the Graphics InterFace (GIF) channel to talk to the GS
  dmaKit_chan_init(DMA_CHANNEL_GIF);
}

static GSGLOBAL *init_gs(void)
{
  GSGLOBAL *gsGlobal = gsKit_init_global();

  gsGlobal->Mode = GS_MODE_NTSC;
  gsGlobal->Interlace = GS_INTERLACED;        // enable Interlaced
  gsGlobal->Width = SCREEN_WIDTH;
  gsGlobal->Height = SCREEN_HEIGHT;
  gsGlobal->Field = GS_FIELD;                 // in interlaced output, read alternating framebuffer rows per field
  gsGlobal->PSM = GS_PSM_CT24;                // framebuffer color format (RGB24)
  gsGlobal->DoubleBuffering = GS_SETTING_ON;  // use double buffer
  gsGlobal->ZBuffering = GS_SETTING_OFF;      // do not allocate/use a depth buffer
  gsGlobal->PSMZ = GS_PSMZ_16S;               // depth buffer format (unused here)
  gsGlobal->PrimAlphaEnable = GS_SETTING_OFF; // disable primitive alpha blending

  gsKit_init_screen(gsGlobal);

  return gsGlobal;
}

static void init_iop_and_load_modules(void)
{
  // Init EE-side RPC protocol to talk to the IOP
  sceSifInitRpc(0);

  // Asks IOP to reset
  while(!SifIopReset("", 0));
  // Busy wait until IOP complete it's reboot
  while(!SifIopSync());

  // Re-init RPC protocol communication with fresh booted IOP
  sceSifInitRpc(0);

  int ret;

  ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
  if (ret < 0)
  {
    printf("Error loading SIO2MAN module from rom0.\n");
    SleepThread();
  }

  ret = SifLoadModule("rom0:PADMAN", 0, NULL);
  if (ret < 0)
  {
    printf("Error loading PADMAN module from rom0.\n");
    SleepThread();
  }
}

static void init_libpad(void)
{
  int ret;

  ret = padInit(0);
  if (ret != 1)
  {
    printf("Failed to initialize libpad: %d\n", ret);
    SleepThread();
  }

  // Allocating 64-byte aligned memory for libpad
  void *padBuffer = memalign(64, 256);

  // Port 0 -> Connector 1 | port 1 -> Connector 2
  // Slot = 0 if not using multitap
  ret = padPortOpen(0, 0, padBuffer);
  if (ret == 0)
  {
    printf("Failed to open port (0,0): %d\n", ret);
    SleepThread();
  }
}

static void read_pads(struct padButtonStatus *buttons)
{
  static int ret, last_state;
  
  // Get the current status of the controller using DMA
  ret = padGetState(0, 0);
  last_state = -1;
  // We will only read if the controller is fully connected and stable
  while (ret != PAD_STATE_STABLE)
  {
    if (ret != last_state)
    {
      if (ret == PAD_STATE_DISCONN)
      {
        printf("Pad(0, 0) is disconnected!\n");
      }
    }
    last_state = ret;
    ret = padGetState(0, 0);

    gsKit_vsync_wait();
  }

  // Read the data into our 'buttons' struct
  ret = padRead(0, 0, buttons);
  if (ret == 0)
  {
    printf("Failed to read pads: %d\n", ret);
  }
}

int main(void)
{
  // Initialize EE DMAC
  init_dmac();
  // Initialize the GS with our settings
  GSGLOBAL *gsGlobal = init_gs();
  // Initializes the IO processor and loads required modules
  init_iop_and_load_modules();

  // Initialize libpad and allocate buttons structure
  init_libpad();
  struct padButtonStatus buttons;

  // Set up the persistent queue to always clear the screen
  gsKit_mode_switch(gsGlobal, GS_PERSISTENT);
  gsKit_clear(gsGlobal, COLOR_BLACK);

  // Go back to one-shot queue
  gsKit_mode_switch(gsGlobal, GS_ONESHOT);

  // Creating our square in the middle of the screen
  Square sq = {
      .x = SCREEN_WIDTH / 2.0f - SQUARE_SIZE / 2.0f,
      .y = SCREEN_HEIGHT / 2.0f - SQUARE_SIZE / 2.0f,
      .size = SQUARE_SIZE,
      .color = COLOR_PURPLE
  };

  while (1)
  {
    read_pads(&buttons);
    process_inputs_square(&buttons, &sq);
    
    // Draw the square
    draw_square(gsGlobal, &sq);

    // Flush the queued commands to the GS via DMA
    gsKit_queue_exec(gsGlobal);
    // Waits VSync and then flips the framebuffers
    gsKit_sync_flip(gsGlobal);
  }

  return 0;
}