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
#include <math.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

#define COLOR_BLACK   GS_SETREG_RGBAQ(0, 0, 0, 0, 0)
#define COLOR_PURPLE  GS_SETREG_RGBAQ(105, 15, 184, 0, 0)
#define COLOR_GREEN   GS_SETREG_RGBAQ(10, 190, 20, 0, 0)
#define COLOR_BLUE    GS_SETREG_RGBAQ(6, 35, 186, 0, 0)

#define TRIAN_HEIGHT      100.0f
#define TRIAN_WIDTH       150.0f
#define TRIAN_MAX_VEL     200.0f
#define TRIAN_ACCEL       600.0f
#define TRIAN_FRICT       1000.0f

typedef struct
{
  float x, y;
  float height, width;
  u64 color_1, color_2, color_3;
  float velocity_x, velocity_y;
} Triangle;

typedef struct
{
  float dir_x, dir_y;
} Input;

static void reset_input_struct(Input *in)
{
  in->dir_x = 0;
  in->dir_y = 0;
}

static float clamp(const float val, const float min, const float max)
{
  if (val > max)
  {
    return max;
  }
  else if (val < min)
  {
    return min;
  }
  else
  {
    return val;
  }
}

static float sign(const float val)
{
  if (val < 0)
  {
    return -1;
  }
  else
  {
    return 1;
  }
}

static float calculate_square_velocity(const float dir, const float curr_vel, const float dt)
{
  float new_vel;

  if (dir != 0)
  {
    new_vel = curr_vel + (dir * TRIAN_ACCEL * dt);
    return clamp(new_vel, -TRIAN_MAX_VEL, TRIAN_MAX_VEL);
  }
  else
  {
    new_vel = fabsf(curr_vel) - (TRIAN_FRICT * dt);
    return (new_vel > 0 ? sign(curr_vel) * new_vel : 0);
  }
}

static void update_square(const Input *in, Triangle *t)
{
  // Since I KNOW this is running in NTSC at 60Hz (synchronized by vsync), I
  // can assume a constant delta time of 0.0167 (1/60). In a more complex app
  // I would probably use a timer to get more precise, but in a simple learning
  // program this'd be overkill 
  const float dt = 0.0167f;

  // Calculating velocities
  t->velocity_x = calculate_square_velocity(in->dir_x, t->velocity_x, dt);
  t->velocity_y = calculate_square_velocity(in->dir_y, t->velocity_y, dt);

  // Clamping velocities so diagonals aren't faster
  float total_speed = sqrtf(t->velocity_x * t->velocity_x + t->velocity_y * t->velocity_y);
  if (total_speed > TRIAN_MAX_VEL)
  {
    float reduction_factor = TRIAN_MAX_VEL / total_speed;
    t->velocity_x *= reduction_factor;
    t->velocity_y *= reduction_factor;
  }

  // Updating positions
  t->x += t->velocity_x * dt;
  t->y += t->velocity_y * dt;
}

static void draw_square(GSGLOBAL *gs, const Triangle *t)
{
  gsKit_prim_triangle_gouraud(
    gs,
    t->x, t->y,
    t->x + t->width, t->y,
    t->x + t->width / 2.0f, t->y - t->height,
    1,
    t->color_1, t->color_2, t->color_3
  );
}

static void get_input_from_buttons(const struct padButtonStatus *buttons, Input *in)
{
  int buttons_status = 0xFFFF ^ buttons->btns;
  
  if (buttons_status & PAD_UP)
  {
    in->dir_y = -1;
  }
  else if (buttons_status & PAD_DOWN)
  {
    in->dir_y = 1;
  }

  if (buttons_status & PAD_RIGHT)
  {
    in->dir_x = 1;
  }
  else if (buttons_status & PAD_LEFT)
  {
    in->dir_x = -1;
  }

  // Normalize direction vector if needed
  if (in->dir_x != 0 && in->dir_y != 0)
  {
    float len = sqrtf(in->dir_x * in->dir_x + in->dir_y * in->dir_y);
    in->dir_x /= len;
    in->dir_y /= len;
  }
}

static void init_iop_and_load_modules(void)
{
  // Init EE-side RPC protocol to talk to the IOP
  SifInitRpc(0);

  // Asks IOP to reset
  while(!SifIopReset("", 0));
  // Busy wait until IOP complete it's reboot
  while(!SifIopSync());

  // Re-init RPC protocol communication with fresh booted IOP
  SifInitRpc(0);

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

// This function returns 1 if it could read pad(0,0) successfully
static u8 read_pads(struct padButtonStatus *buttons)
{
  // Get the current status of the controller
  int state = padGetState(0, 0);

  // If the pad is not stable, we don't read it and return 0
  if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1)
  {
    return 0;
  }

  // If the pad is stable, we read the state and return 1
  return padRead(0, 0, buttons) != 0;
}

int main(void)
{
  // Initializes the IO processor and loads required modules
  init_iop_and_load_modules();
  // Initialize EE DMAC
  init_dmac();
  // Initialize the GS with our settings
  GSGLOBAL *gsGlobal = init_gs();

  // Initialize libpad and allocate buttons structure
  init_libpad();
  struct padButtonStatus buttons;
  Input in;

  // Set up the persistent queue to always clear the screen
  gsKit_mode_switch(gsGlobal, GS_PERSISTENT);
  gsKit_clear(gsGlobal, COLOR_BLACK);

  // Go back to one-shot queue
  gsKit_mode_switch(gsGlobal, GS_ONESHOT);

  // Creating our triangle in the middle of the screen
  Triangle t = {
      .x = SCREEN_WIDTH / 2.0f - TRIAN_WIDTH / 2.0f,
      .y = SCREEN_HEIGHT / 2.0f + TRIAN_HEIGHT / 2.0f,
      .width = TRIAN_WIDTH,
      .height = TRIAN_HEIGHT,
      .color_1 = COLOR_PURPLE,
      .color_2 = COLOR_GREEN,
      .color_3 = COLOR_BLUE,
      .velocity_x = 0,
      .velocity_y = 0
  };

  while (1)
  {
    reset_input_struct(&in);

    if (read_pads(&buttons))
    {
      get_input_from_buttons(&buttons, &in);
    }

    update_square(&in, &t);
    
    // Draw the triangle
    draw_square(gsGlobal, &t);

    // Flush the queued commands to the GS via DMA
    gsKit_queue_exec(gsGlobal);
    // Wait for the queue to render
    gsKit_finish();
    // Waits VSync and then flips the framebuffers
    gsKit_sync_flip(gsGlobal);
  }

  return 0;
}