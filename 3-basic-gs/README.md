# Basics of PS2's Graphics Synthesizer

## dmaKit and gsKit

The `dmaKit` is a library that provides functions for managing Direct Memory Access (DMA) transfers on the PlayStation 2, while `gsKit` is a library that provides functions for rendering graphics using the Graphics Synthesizer (GS) of the PlayStation 2. Together, these libraries allow developers to efficiently manage memory and render graphics on the PS2.

## What the program does (step-by-step)

1. Initializes the DMA Controller (DMAC) so `gsKit` can send drawing commands to the Graphics Synthesizer (GS) via the GIF (Graphics Interface) channel.

2. Allocates and configures a `GSGLOBAL` struct, which holds all gsKit state: resolution, framebuffer addresses in VRAM, pixel formats, interlace/field mode, etc.

3. Calls `gsKit_init_screen()`, which:
  
    - Resets the GS and sets up the CRTC (display timing).

    - Allocates framebuffer(s) in GS VRAM.

    - Programs GS registers (FRAME, DISPLAY, SCISSOR, TEST, ZBUF, etc.) for both contexts.

    - Sends this initialization packet to the GS over the GIF DMA channel.

4. Switches to “oneshot” mode, which affects how gsKit’s drawing queue behaves when you execute it.

5. In the main loop:

    - Queues a clear command (`gsKit_clear`) with a given RGBAQ color.

    - Flushes the queue to the GS via DMA (`gsKit_queue_exec`).

    - Waits for vsync (`gsKit_vsync_wait`) to avoid tearing and to pace your frame rate.
