#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

int main(void)
{
  // Initialize the DMA Controller.
  // gsKit needs this to send drawing commands/data to the GS over DMA.
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
  // Allocate and configure a GSGLOBAL struct.
  // This is the core structure for gsKit operations.
  // All relevant data to GS operation including framebuffer pointers,
  // display mode, dimensions, offsets, and GS register values are maintained
  // here.
  // Most gsKit functions will require the pointer of gsGlobal as their first
  // parameter.
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

  // This will initialize the GS hardware: sets up the CRTC,
  // allocates the framebuffer(s) in GS VRAM, resets the GS and
  // initializes the display and drawing environments
  gsKit_init_screen(gsGlobal);

  // Select the transient command queue
  // This is independent of framebuffer double buffering
  gsKit_mode_switch(gsGlobal, GS_ONESHOT);

  // Setting clear color to red (RGB)
  u8 r = 255, g = 0, b = 0;

  while (1)
  {
    // ---
    // Everything between here and gsKit_queue_exec are drawing
    // commands queued up, not yet sent to the GS.
    // ---

    // A clear screen command
    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(r, g, b, 0x00, 0x00));

    // Flush the queued commands to the GS via DMA.
    gsKit_queue_exec(gsGlobal);

    // Blocks until vsync
    gsKit_vsync_wait();
  }

  return 0;
}