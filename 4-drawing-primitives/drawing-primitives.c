#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

int main(void)
{
  // Setting colors
  const u64 red_color = GS_SETREG_RGBAQ(255, 0, 0, 0, 0);
  const u64 blue_color = GS_SETREG_RGBAQ(0, 0, 255, 0, 0);

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

  GSGLOBAL *gsGlobal;
  gsGlobal = gsKit_init_global();

  gsGlobal->Mode = GS_MODE_NTSC;              // or GS_MODE_PAL for PAL setups
  gsGlobal->Interlace = GS_INTERLACED;        // enable Interlaced
  gsGlobal->Width = 640;                      // 640px with
  gsGlobal->Height = 448;                     // 448px height (not 480px because of CRT overscan - common practice in PS2 games)
  gsGlobal->Field = GS_FIELD;                 // in interlaced output, read alternating framebuffer rows per field
  gsGlobal->PSM = GS_PSM_CT24;                // framebuffer color format (RGB24)
  gsGlobal->PSMZ = GS_PSMZ_16S;               // depth buffer format (unused here)
  gsGlobal->DoubleBuffering = GS_SETTING_OFF; // no double buffering, use only one GS framebuffer for drawing and display
  gsGlobal->ZBuffering = GS_SETTING_OFF;      // do not allocate/use a depth buffer
  gsGlobal->PrimAlphaEnable = GS_SETTING_OFF; // disable primitive alpha blending

  gsKit_init_screen(gsGlobal);

  gsKit_mode_switch(gsGlobal, GS_PERSISTENT);

  gsKit_clear(gsGlobal, red_color);

  const float center_y = 224.0f;
  const float center_x = 320.0f;

  const float triangle_width = 540.0f;
  const float triangle_height = 300.0f;

  gsKit_prim_triangle(gsGlobal, 
    -triangle_width / 2 + center_x, triangle_height / 2 + center_y,
    triangle_width / 2 + center_x, triangle_height / 2 + center_y,
    center_x, -triangle_height / 2 + center_y,
    1,
    blue_color);

  while (1)
  {
    // Flush the queued commands to the GS via DMA.
    gsKit_queue_exec(gsGlobal);

    // Blocks until vsync
    gsKit_vsync_wait();
  }

  return 0;
}