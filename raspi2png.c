//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2014 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

// modified raspi2png, captures bottom half of display and copies to top

#include <stdio.h>
#include <assert.h>

#include "bcm_host.h"

#include <pigpio.h>

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

int
main(
    int argc,
    char *argv[])
{
    VC_IMAGE_TYPE_T imageType = VC_IMAGE_RGBA32;
    int8_t dmxBytesPerPixel  = 4;

    int result = 0;

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T displayHandle
        = vc_dispmanx_display_open(0);

    assert(displayHandle != 0);

    DISPMANX_MODEINFO_T modeInfo;
    result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

    assert((result == 0) || !"modeInfo");

    int32_t dmxWidth = modeInfo.width;
    int32_t dmxHeight = modeInfo.height;

    int32_t dmxPitch = dmxBytesPerPixel * ALIGN_TO_16(dmxWidth);

    void *dmxImagePtr = malloc(dmxPitch * dmxHeight);

    assert(dmxImagePtr != NULL);

    uint32_t vcImagePtr = 0;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(imageType,
                                                 dmxWidth,
                                                 dmxHeight,
                                                 &vcImagePtr);

  assert(gpioInitialise() >= 0);
  gpioSetMode(18, PI_INPUT);

  while (1)
  { 
    while (gpioRead(18) == 0) {}
    while (gpioRead(18) == 1) {}

    result = vc_dispmanx_snapshot(displayHandle,
                                  resourceHandle,
                                  DISPMANX_NO_ROTATE);

    assert((result == 0) || !"snapshot");

    VC_RECT_T rect;
    result = vc_dispmanx_rect_set(&rect, 0, 0, dmxWidth, dmxHeight);

    assert((result == 0) || !"rect_set");

    result = vc_dispmanx_resource_read_data(resourceHandle,
                                            &rect,
                                            dmxImagePtr,
                                            dmxPitch);


    assert((result == 0) || !"read_data");

    FILE *tgt = fopen("/dev/fb0", "wb");
    for(char *p=dmxImagePtr + dmxPitch*dmxHeight - 2; p > (char*)dmxImagePtr + dmxPitch*dmxHeight/2; p-=4)
    {
      p[-2]^=*p; *p^=p[-2]; p[-2]^=*p;
    }
    fwrite(dmxImagePtr + dmxPitch*dmxHeight/2, dmxPitch, dmxHeight/2, tgt);
    fclose(tgt);
  }

    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    free(dmxImagePtr);

    return 0;
}

