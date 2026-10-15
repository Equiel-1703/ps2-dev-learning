#include <dmaKit.h>
#include <gsKit.h>

int main()
{
  dmaKit_init(
    D_CTRL_RELE_OFF,
    D_CTRL_MFD_OFF,
    D_CTRL_STS_UNSPEC,
    D_CTRL_STD_OFF,
    D_CTRL_RCYC_8,
    1 << DMA_CHANNEL_GIF);

  
  return 0;
}