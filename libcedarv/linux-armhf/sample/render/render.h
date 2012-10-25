#ifndef RENDER_H
#define RENDER_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <libcedarv.h>
#include "drv_display_sun4i.h"

#define OUTPUT_IN_YUVPLANNER_FORMAT
#define USE_MP_TO_TRANSFORM_PIXEL

#define HDMI_OUTPUT 1
#ifdef __cplusplus
extern "C" {
#endif
typedef struct VIDEO_RENDER_CONTEXT_TYPE
{
    int                 de_fd;
    int             	de_layer_hdl;
    int             	vid_wind_ratio;
    __disp_rect_t       video_window;
    __disp_rect_t       src_frm_rect;
    int            		pic_pix_ratio;

    unsigned int        layer_open_flag;
    unsigned int        first_frame_flag;

    unsigned int		hdmi_mode;

#ifdef OUTPUT_IN_YUVPLANNER_FORMAT
    void*               ybuf;
    void*               ubuf;
    void*               vbuf;

#ifdef USE_MP_TO_TRANSFORM_PIXEL
    int                 scaler_hdl;
#endif
#endif

} VIDEO_RENDER_CONTEXT_TYPE;

extern int render_init();
extern void render_exit(void);
extern int render_render(void *frame_info, int frame_id);
extern int render_get_disp_frame_id(void);
extern int render_set_screen_rect(__disp_rect_t rect);
extern void render_get_screen_rect(__disp_rect_t *rect);


#ifdef OUTPUT_IN_YUVPLANNER_FORMAT

    enum FORMAT_CONVERT_COLORFORMAT
    {
    	CONVERT_COLOR_FORMAT_NONE = 0,
    	CONVERT_COLOR_FORMAT_YUV420PLANNER,
    	CONVERT_COLOR_FORMAT_YUV422PLANNER,
    	CONVERT_COLOR_FORMAT_YUV420MB,
    	CONVERT_COLOR_FORMAT_YUV422MB,
    };

    typedef struct ScalerParameter
    {
    	int mode; //0: YV12 1:thumb yuv420p
    	int format_in;
    	int format_out;

    	int width_in;
    	int height_in;

    	int width_out;
    	int height_out;

    	void *addr_y_in;
    	void *addr_c_in;
    	unsigned int addr_y_out;
    	unsigned int addr_u_out;
    	unsigned int addr_v_out;
    }ScalerParameter;

    int HardwarePictureScaler(ScalerParameter *cdx_scaler_para);
    int SoftwarePictureScaler(ScalerParameter *cdx_scaler_para);
    int TransformPicture(cedarv_picture_t* p);
#endif

#ifdef __cplusplus
}
#endif

#endif
