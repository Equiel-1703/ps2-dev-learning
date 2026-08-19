#include <tamtypes.h>
#include <kernel.h>
#include <gsKit.h>
#include <dmaKit.h>

int main()
{
  // Initialize the DMA Controller — gsKit needs this to send
  // drawing commands/data to the GS over DMA.
  dmaKit_init(
      D_CTRL_RELE_OFF,
      D_CTRL_MFD_OFF,
      D_CTRL_STS_UNSPEC,
      D_CTRL_STD_OFF,
      D_CTRL_RCYC_8,
      1 << DMA_CHANNEL_GIF);
  dmaKit_chan_init(DMA_CHANNEL_GIF);

  // Allocate and configure a GSGLOBAL struct — this holds all the
  // state gsKit needs: resolution, framebuffer addresses, etc.
  GSGLOBAL *gsGlobal;
  gsGlobal = gsKit_init_global();

  // Optional but recommended while learning: draw a wireframe
  // primitive outline isn't needed here, but you can tweak mode:
  gsGlobal->Mode = GS_MODE_NTSC; // or GS_MODE_PAL for PAL setups
  gsGlobal->Interlace = GS_INTERLACED;
  gsGlobal->Width = 640;
  gsGlobal->Height = 448;
  gsGlobal->Field = GS_FIELD;
  gsGlobal->PSM = GS_PSM_CT24;  // framebuffer color format (RGB24)
  gsGlobal->PSMZ = GS_PSMZ_16S; // depth buffer format (unused here)
  gsGlobal->DoubleBuffering = GS_SETTING_OFF;
  gsGlobal->ZBuffering = GS_SETTING_OFF;
  gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;

  // This actually talks to the GS hardware: sets up the CRTC,
  // allocates the framebuffer(s) in GS VRAM, resets the GIF path.
  gsKit_init_screen(gsGlobal);

  // Optional: turn off the "Sony" boot logo flicker-fix delay, etc.
  gsKit_mode_switch(gsGlobal, GS_ONESHOT);

  // Pick your clear color (R, G, B, Alpha, plus a "Q" term you can ignore for now)
  // Values are 0-255 per channel.
  u8 r = 255, g = 0, b = 0;

  while (1)
  {
    // Everything between here and gsKit_queue_exec is drawing
    // commands queued up, not yet sent to the GS.
    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(r, g, b, 0x00, 0x00));

    // Flush the queued primitives to the GS via DMA.
    gsKit_queue_exec(gsGlobal);

    // Blocks until vsync
    gsKit_vsync_wait();
  }

  return 0;
}