// define decode param and some system header file by chip and os.
#ifndef _CHIP_ADAPTER_H_
#define _CHIP_ADAPTER_H_

#include "libve_typedef.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    #define MAX_SUPPORTED_VIDEO_WIDTH         3840
    #define MAX_SUPPORTED_VIDEO_HEIGHT        2160
    #define VDECODER_SUPPORT_ANAGLAGH_TRANSFORM 1
    #define VDECODER_SUPPORT_MAF                1

#ifdef __cplusplus
}
#endif

#endif  /* _CHIP_ADAPTER_H_ */

