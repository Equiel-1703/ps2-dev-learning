#include <tamtypes.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <stdio.h>
#include <malloc.h>
#include <kernel.h>
#include <math.h>
#include <string.h>

#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       448

// 0x80 = transparent
// 0x00 = opaque
#define COLOR_BLUE          GS_SETREG_RGBAQ(0, 64, 211, 0, 0)
#define TEXTURE_COLOR       GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0)
#define TEXTURE_PATH        "img/sia.png"

#define SQUARE_SIZE         128.0f
#define SQUARE_MIN_SIZE     10.0f
#define SQUARE_MAX_SIZE     448.0f
#define SQUARE_MAX_VEL      200.0f
#define SQUARE_ACCEL        600.0f
#define SQUARE_FRICT        1000.0f

#define DEAD_ZONE_THRESHOLD 0.25f
#define DEAD_ZONE_CHECK(analog_axis) ((analog_axis) > -DEAD_ZONE_THRESHOLD && (analog_axis) < DEAD_ZONE_THRESHOLD)

typedef struct
{
  float x, y;
  float size;
  GSTEXTURE *tex;
  float velocity_x, velocity_y;
} Square;

typedef struct
{
  float dir_x, dir_y;
  float scale;
} Input;

static void reset_input_struct(Input *in)
{
  in->dir_x = 0.0f;
  in->dir_y = 0.0f;
  in->scale = 0.0f;
}

static inline float clamp(const float val, const float min, const float max)
{
  if (val < min)
  {
    return min;
  }
  else if (val > max)
  {
    return max;
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
  if (dir != 0.0f)
  {
    return curr_vel + (dir * SQUARE_ACCEL * dt);
  }
  else
  {
    float new_vel = fabsf(curr_vel) - (SQUARE_FRICT * dt);
    return (new_vel > 0.0f ? sign(curr_vel) * new_vel : 0.0f);
  }
}

// Calculates new position and scale for the sqare based on inputs
static void update_square(const Input *in, Square *sq)
{
  // Since I KNOW this is running in NTSC at 60Hz (synchronized by vsync), I
  // can assume a constant delta time of 0.0167 (1/60). In a more complex app
  // I would probably use a timer to get more precise, but in a simple learning
  // program this'd be overkill 
  const float dt = 0.0167f;

  // Calculating velocities
  sq->velocity_x = calculate_square_velocity(in->dir_x, sq->velocity_x, dt);
  sq->velocity_y = calculate_square_velocity(in->dir_y, sq->velocity_y, dt);

  // Clamping velocities so diagonals aren't faster
  float total_speed = sqrtf(sq->velocity_x * sq->velocity_x + sq->velocity_y * sq->velocity_y);
  if (total_speed > SQUARE_MAX_VEL)
  {
    float reduction_factor = SQUARE_MAX_VEL / total_speed;
    sq->velocity_x *= reduction_factor;
    sq->velocity_y *= reduction_factor;
  }

  // Updating position
  sq->x += sq->velocity_x * dt;
  sq->y += sq->velocity_y * dt;

  // Updating scale
  float old_size = sq->size;
  sq->size = clamp(
    sq->size + in->scale * SQUARE_ACCEL * dt, 
    SQUARE_MIN_SIZE, 
    SQUARE_MAX_SIZE);

  // If resized, center the square
  if (old_size != sq->size)
  {
    float diff = old_size - sq->size;

    sq->x += diff / 2.0f;
    sq->y += diff / 2.0f;
  }

  // Clamping position inside screen
  sq->x = clamp(sq->x, 0.0f, SCREEN_WIDTH - sq->size);
  sq->y = clamp(sq->y, 0.0f, SCREEN_HEIGHT - sq->size);
}

static void draw_square(GSGLOBAL *gs, const Square *sq)
{
  // Signalize we are going to use the sqare's texture
  gsKit_TexManager_bind(gs, sq->tex);

  // Render the square
  gsKit_prim_sprite_texture(
    gs,
    sq->tex,
    sq->x, sq->y,
    0, 0,
    sq->x + sq->size, sq->y + sq->size,
    sq->tex->Width, sq->tex->Height,
    1,
    TEXTURE_COLOR
  );
}

// Reading analog sticks
static void get_input_from_buttons(const struct padButtonStatus *buttons, Input *in)
{
  // Horizontally:  0 is full left and 255 is full right.
  // Vertically:    0 is full up and 255 is full down
  float lanalog_h = (float)(buttons->ljoy_h - 128) / 128.0f;
  float lanalog_v = (float)(buttons->ljoy_v - 128) / 128.0f;
  float ranalog_v = -(float)(buttons->rjoy_v - 128) / 128.0f;

  // Dead zone (ignoring small values)
  lanalog_h = DEAD_ZONE_CHECK(lanalog_h) ? 0.0f : lanalog_h;
  lanalog_v = DEAD_ZONE_CHECK(lanalog_v) ? 0.0f : lanalog_v;
  ranalog_v = DEAD_ZONE_CHECK(ranalog_v) ? 0.0f : ranalog_v;

  in->dir_x = clamp(lanalog_h, -1.0f, 1.0f);
  in->dir_y = clamp(lanalog_v, -1.0f, 1.0f);
  in->scale = clamp(ranalog_v, -1.0f, 1.0f);
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
  gsGlobal->PrimAlphaEnable = GS_SETTING_ON;  // Enable primitive alpha blending

  gsKit_init_screen(gsGlobal);

  return gsGlobal;
}

static void init_libpad(void)
{
  int ret, i;

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

  // Wait controller stabilize
  while (padGetState(0, 0) != PAD_STATE_STABLE);

  // Check number of modes this controller have
  const int n_modes = padInfoMode(0, 0, PAD_MODETABLE, -1);
  for (i = 0; i < n_modes; i++)
  {
    // Check if it has DualShock mode
    if (padInfoMode(0, 0, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
    {
      // Switch to analog mode and lock it, so the analog button
      // on the controller can't toggle it back to digital.
      padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
      
      // Wait controller stabilize after changing it's mode
      while (padGetState(0, 0) != PAD_STATE_STABLE);
      return;
    }
  }

  // If we get here, the controller does not have DualShock mode
  printf("Looks like your controller is not a DualShock controller!\n");
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

  // Creating our texture
  GSTEXTURE sq_tex;
  memset(&sq_tex, 0, sizeof(GSTEXTURE));
  
  sq_tex.Delayed = 1;
  if (gsKit_texture_png(gsGlobal, &sq_tex, TEXTURE_PATH) < 0)
  {
    printf("Failed to load image from '%s'\n", TEXTURE_PATH);
    SleepThread();
  }
  sq_tex.Filter = GS_FILTER_NEAREST;

  printf("Loaded texture size: %d x %d\n", sq_tex.Width, sq_tex.Height);
  printf("Texture filter: %s\n", sq_tex.Filter == GS_FILTER_NEAREST ? "Nearest" : "Linear");
  printf("PSM: %s\n", sq_tex.PSM == GS_PSM_CT32 ? "CT32" : "other"); 

  // Creating our square in the middle of the screen
  Square sq = {
      .x = SCREEN_WIDTH / 2.0f - SQUARE_SIZE / 2.0f,
      .y = SCREEN_HEIGHT / 2.0f - SQUARE_SIZE / 2.0f,
      .size = SQUARE_SIZE,
      .tex = &sq_tex,
      .velocity_x = 0,
      .velocity_y = 0
  };

  // Enter one-shot queue
  gsKit_mode_switch(gsGlobal, GS_ONESHOT);

  // Set blending equation: (Cd - Cs) * As + Cs
  // Cd - Color Destination (canvas)
  // Cs - Color Source (the texture)
  // As - Alpha Source (texture alpha)
  gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);

  // Initialize texture manager
  gsKit_TexManager_init(gsGlobal);

  while (1)
  {
    gsKit_clear(gsGlobal, COLOR_BLUE);

    reset_input_struct(&in);
    if (read_pads(&buttons))
    {
      get_input_from_buttons(&buttons, &in);
    }
    update_square(&in, &sq);
    
    // Draw the square
    draw_square(gsGlobal, &sq);
    
    // Flush the queued commands to the GS via DMA
    gsKit_queue_exec(gsGlobal);
    // Wait for the queue to render
    gsKit_finish();
    // Waits VSync and then flips the framebuffers
    gsKit_sync_flip(gsGlobal);
    // Signalize texture manager we are done rendering this frame
    gsKit_TexManager_nextFrame(gsGlobal);
  }

  return 0;
}