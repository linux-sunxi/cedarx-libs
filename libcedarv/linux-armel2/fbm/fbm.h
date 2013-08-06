
#ifndef FBM_H
#define FBM_H

#include "libve_typedef.h"
#include "os_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

    #define FBM_MAX_FRAME_NUM           64
    #define FBM_REQUEST_FRAME_WAIT_TIME 10
    #define FBM_MUTEX_WAIT_TIME         5000
    
    typedef struct FRAME_INFO frame_info_t;
    
    typedef enum FRAME_STATUS
    {
        FS_EMPTY                         = 0,
        FS_DECODER_USING                 = 1,
        FS_DISPLAY_USING                 = 2,
        FS_DECODER_AND_DISPLAY_USING     = 3,
        FS_DECODER_USING_DISPLAY_DISCARD = 4
    }frame_status_e;

    struct FRAME_INFO
    {
        frame_status_e status;
        vpicture_t     picture;
        frame_info_t*  next;
    };
    
    typedef struct FRAME_BUFFER_MANAGER
    {
        u32             max_frame_num;
        
        frame_info_t*   empty_queue;
        frame_info_t*   display_queue;
        
        Handle          mutex;
        
        frame_info_t    frames[FBM_MAX_FRAME_NUM];

        s32				is_preview_mode;

    }fbm_t;
    
    Handle      fbm_init_ex(u32 max_frame_num,
    					 u32 min_frame_num,
    					 u32 size_y[],
    					 u32 size_u[],
    					 u32 size_v[],
    					 u32 size_alpha[],
    					 _3d_mode_e _3d_mode,
    					 pixel_format_e format,
    					 u8		is_preview_mode,
                         void* parent);
                         
    void        fbm_release(Handle h, void* parent);
    s32         fbm_reset(u8 reserved_frame_index, Handle h);
    s32			fbm_flush(Handle h);
    
    vpicture_t* fbm_decoder_request_frame(Handle h);
    void        fbm_decoder_return_frame(vpicture_t* frame, u8 valid, Handle h);
    void        fbm_decoder_share_frame(vpicture_t* frame, Handle h);
    
    vpicture_t* fbm_display_request_frame(Handle h);
    void        fbm_display_return_frame(vpicture_t* frame, Handle h);
    void 		fbm_vdecoder_return_frame(vpicture_t * frame, Handle h);
    vpicture_t* fbm_display_pick_frame(Handle h);
    
    vpicture_t* fbm_index_to_pointer(u8 index, Handle h);
    u8          fbm_pointer_to_index(vpicture_t* frame, Handle h);
    s32         fbm_alloc_redBlue_frame_buffer(Handle h);
    s32         fbm_free_redBlue_frame_buffer(Handle h);
    s32         fbm_alloc_rotate_frame_buffer(Handle h);
    void 		fbm_flush_frame(Handle h, s64 pts);
    void        fbm_print_status(Handle h);
#ifdef __cplusplus
}
#endif

#endif

