
#ifndef VBV_H
#define VBV_H

#include "libve_typedef.h"
#include "os_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef void (*free_buffer_sem_cb)(void* handle);

    Handle vbv_init(u32 vbv_size, u32 max_frame_num);
    
    void   vbv_release(Handle vbv);
    
    s32    vbv_request_buffer(u8** buf, u32* buf_size, u32 require_size, Handle vbv);
    
    s32    vbv_add_stream(vstream_data_t* stream, Handle vbv);
    
    vstream_data_t* vbv_request_stream_frame(Handle vbv);
    
    void   vbv_return_stream_frame(vstream_data_t* stream, Handle vbv);
    
    void   vbv_flush_stream_frame(vstream_data_t* stream, Handle vbv);
    
    u32    vbv_get_stream_num(Handle vbv);
    
    u8*    vbv_get_base_addr(Handle vbv);
    
    u32    vbv_get_buffer_size(Handle vbv);
    
    void   vbv_reset(Handle vbv);
    
    u8*    vbv_get_current_write_addr(Handle vbv);
    
    u32    vbv_get_valid_data_size(Handle vbv);
    
    u32    vbv_get_valid_frame_num(Handle vbv);
    
    u32    vbv_get_max_stream_frame_num(Handle vbv);

    void   vbv_set_parent(void* parent, Handle vbv);

    void*  vbv_get_parent(Handle vbv);

    void vbv_free_vbs_sem(Handle vbv);

    void vbv_set_free_vbs_sem_cb(free_buffer_sem_cb cb, Handle vbv);

#ifdef __cplusplus
}
#endif

#endif

