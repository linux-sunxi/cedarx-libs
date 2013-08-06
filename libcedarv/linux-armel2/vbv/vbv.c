
#include "commom_type.h"
#include "vbv.h"

#define __msg(msg...) {printf("%s:%d   ", __FILE__, __LINE__); printf(msg); printf("\n");}
#define VBV_MUTEX_WAIT_TIME 500     //* wait mostly 500 ms for vbv mutex.

typedef struct STREAM_FRAME stream_frame_t;
struct STREAM_FRAME
{
    vstream_data_t  vstream;
    u32             id;
    stream_frame_t* next;
};

typedef struct STREAM_FIFO
{
    stream_frame_t* in_frames;
    u32             read_index;
    u32             write_index;
    u32             frame_num;
    u32             max_frame_num;
}stream_fifo_t;

typedef struct STREAM_QUEUE
{
    u32             frame_num;
    stream_frame_t* head;
}stream_queue_t;

typedef struct VBV_MANAGER
{
    Handle          mutex;
    u8*             vbv_buf;
    u8*             vbv_buf_end;
    u8*             read_addr;
    u8*             write_addr;
    u32             valid_size;
    u32             max_size;
    
    stream_fifo_t   frame_fifo;
    stream_queue_t  frame_queue;

    free_buffer_sem_cb free_buffer_sem;
    void* 			parent;
}vbv_t;

static s32 lock(vbv_t* vbv);
static void unlock(vbv_t* vbv);
static void enqueue_to_head(stream_frame_t* stream, stream_queue_t* q);
static void enqueue_to_tail(stream_frame_t* stream, stream_queue_t* q);
static stream_frame_t* dequeue(stream_queue_t* q);


Handle vbv_init(u32 vbv_size, u32 max_frame_num)
{
    u32    i;
    u8*    vbv_buf;
    vbv_t* vbv;
    
    if(vbv_size == 0 || max_frame_num == 0)
        return NULL;
    
    vbv_buf = (u8*)mem_palloc(vbv_size, 1024);
    if (vbv_buf == NULL)
        return NULL;

    vbv = (vbv_t*)mem_alloc(sizeof(vbv_t));
    if (vbv == NULL)
    {
        mem_pfree(vbv_buf);
        return NULL;
    }
    mem_set(vbv, 0, sizeof(vbv_t));

    vbv->frame_fifo.in_frames = (stream_frame_t*)mem_alloc(max_frame_num*sizeof(stream_frame_t));
    if (vbv->frame_fifo.in_frames == NULL)
    {
        mem_free(vbv);
        mem_pfree(vbv_buf);
        return NULL;
    }
    
    mem_set(vbv->frame_fifo.in_frames, 0, max_frame_num*sizeof(stream_frame_t));
    for(i=0; i<max_frame_num; i++)
    {
        vbv->frame_fifo.in_frames[i].id = i;
    }
    
    vbv->mutex = semaphore_create(1);
    if (vbv->mutex == NULL)
    {
        mem_free(vbv->frame_fifo.in_frames);
        mem_free(vbv);
        mem_pfree(vbv_buf);
        return NULL;
    }

    vbv->vbv_buf     = vbv_buf;
    vbv->max_size    = vbv_size;
    vbv->vbv_buf_end = vbv_buf + vbv_size - 1;
    vbv->read_addr   = vbv_buf;
    vbv->write_addr  = vbv->vbv_buf;
    vbv->valid_size  = 0;

    vbv->frame_fifo.frame_num     = 0;
    vbv->frame_fifo.read_index    = 0;
    vbv->frame_fifo.write_index   = 0;
    vbv->frame_fifo.max_frame_num = max_frame_num;
    
    vbv->frame_queue.frame_num = 0;
    vbv->frame_queue.head      = NULL;
    
    return (Handle)vbv;
}


void vbv_release(Handle vbv)
{
    vbv_t* v;
    
    v = (vbv_t*)vbv;
    if (v)
    {
        if (v->mutex != NULL)
        {
        	semaphore_delete(v->mutex, SEM_DEL_ALWAYS);
            v->mutex = NULL;
        }

        if (v->vbv_buf != NULL)
        {
            mem_pfree(v->vbv_buf);
            v->vbv_buf = NULL;
        }

        if (v->frame_fifo.in_frames)
        {
            mem_free(v->frame_fifo.in_frames);
            v->frame_fifo.in_frames = NULL;
        }

        mem_free(v);
    }

    return;	
}

s32 vbv_request_buffer(u8** buf, u32* buf_size, u32 require_size, Handle vbv)
{
    vbv_t* v;
    u32    free_size;
    
    v = (vbv_t*)vbv;

    if (v == NULL || buf == NULL || buf_size == NULL)
    {
        return -1;
    }

    if(lock(v) != 0)
        return -1;

    if (v->frame_fifo.frame_num >= v->frame_fifo.max_frame_num)
    {
        unlock(v);
        return -1;
    }

    if (v->valid_size < v->max_size)
    {
        free_size = v->max_size - v->valid_size;

#ifdef OS_LINUX
		//* the CedarX player requires that buffer size always enough if return success.
        if (require_size > free_size)
        {
        	unlock(v);
            return -1;
        }
#else
        if (require_size > free_size)
            require_size = free_size;
#endif

        *buf = v->write_addr;
        *buf_size = require_size;

        unlock(v);
        return 0;
    }
    else
    {
        unlock(v);
        return -1;
    }
}


s32 vbv_add_stream(vstream_data_t* stream, Handle vbv)
{
    u32    write_index;
    u8*    new_write_addr;
    vbv_t* v;
    
    v = (vbv_t*)vbv;

    if (stream == NULL || v == NULL)
    {
        return -1;
    }

    if(lock(v) != 0)
        return -1;

    if (v->frame_fifo.frame_num >= v->frame_fifo.max_frame_num)
    {
        unlock(v);
        return -1;
    }

    if (stream->length + v->valid_size > v->max_size)
    {
        unlock(v);
        return -1;
    }
    if(stream->valid == 0)
    {   
        stream->valid = 1;
    }
    write_index = v->frame_fifo.write_index;
    mem_cpy(&v->frame_fifo.in_frames[write_index].vstream, stream, sizeof(vstream_data_t));
    enqueue_to_tail(&v->frame_fifo.in_frames[write_index], &v->frame_queue);     //* add this frame to the queue tail.
    write_index++;
    if (write_index >= v->frame_fifo.max_frame_num)
    {
        write_index = 0;
    }

    v->frame_fifo.write_index = write_index;
    v->frame_fifo.frame_num++;
    v->valid_size += stream->length;

    new_write_addr = v->write_addr + stream->length;
    if (new_write_addr > v->vbv_buf_end)
    {
        new_write_addr -= v->max_size;
    }

    v->write_addr = new_write_addr;

    unlock(v);

    return 0;
}


vstream_data_t* vbv_request_stream_frame(Handle vbv)
{
    stream_frame_t* frame;    
    vbv_t*          v;
    
    v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return NULL;
    }

    if (lock(v) != 0)
    {
        return NULL;
    }

    frame = dequeue(&v->frame_queue);
    
    if(frame == NULL)
    {
        unlock(v);
        return NULL;
    }

    unlock(v);

    frame->vstream.id = frame->id;
    return &frame->vstream;
}


void vbv_return_stream_frame(vstream_data_t* stream, Handle vbv)
{
    vbv_t* v;
    s32 delta;
    u32 frame_num;
    
    v = (vbv_t*)vbv;

    if (v == NULL)
    {
        return;
    }

    if (lock(v) != 0)
    {
        return;
    }

    if (v->frame_fifo.frame_num == 0)
    {
        unlock(v);
        return;
    }
    //this is return operation, be careful.
    delta = stream->id - v->frame_fifo.in_frames[v->frame_fifo.read_index].id;
    if(delta < 0)
    {
    	delta = v->frame_fifo.in_frames[v->frame_fifo.read_index].id - stream->id;
    }

    frame_num =( v->frame_fifo.read_index + delta) % v->frame_fifo.max_frame_num;
    enqueue_to_head(&v->frame_fifo.in_frames[frame_num], &v->frame_queue);

    unlock(v);
    return;
}

void vbv_set_free_vbs_sem_cb(free_buffer_sem_cb cb, Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;

    if (v != NULL)
    {
    	v->free_buffer_sem = cb;
    }

    return;
}

void vbv_free_vbs_sem(Handle vbv)
{
    vbv_t* v;

    v = (vbv_t*)vbv;

    if (v == NULL)
    {
        return;
    }

    if (lock(v) != 0)
    {
        return;
    }
    if(v->free_buffer_sem && v->parent)
    {
    	v->free_buffer_sem(v->parent);
    }
    unlock(v);
}

void vbv_flush_stream_frame(vstream_data_t* stream, Handle vbv)
{
    u32    read_index;
    vbv_t* v;
    
    v = (vbv_t*)vbv;

    if (v == NULL)
    {
        return;
    }

    if (lock(v) != 0)
    {
        return;
    }

    if (v->frame_fifo.frame_num == 0)
    {
        unlock(v);
        return;
    }

    read_index = v->frame_fifo.read_index;
    if(stream != &v->frame_fifo.in_frames[read_index].vstream)
    {
        unlock(v);
        return;
    }
    
    read_index++;
    if (read_index == v->frame_fifo.max_frame_num)
    {
        read_index = 0;
    }

    v->frame_fifo.read_index = read_index;
    v->frame_fifo.frame_num--;
    v->valid_size -= stream->length;
    v->read_addr += stream->length;
    if (v->read_addr > v->vbv_buf_end)
    {
        v->read_addr  -= v->max_size;
    }

    unlock(v);
    
    return;
}


u32 vbv_get_stream_num(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return 0;
    }

    return v->frame_queue.frame_num;
}

u8* vbv_get_base_addr(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return NULL;
    }

    return v->vbv_buf;
}
    

u32 vbv_get_buffer_size(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if(v == NULL)
    {
        return 0;
    }
    
    return v->max_size;
}
 
    
void vbv_reset(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return;
    }

    if(lock(v) != 0)
        return;

    v->read_addr = v->write_addr = v->vbv_buf;
    v->valid_size = 0;

    v->frame_fifo.frame_num   = 0;
    v->frame_fifo.read_index  = 0;
    v->frame_fifo.write_index = 0;
    v->frame_queue.frame_num  = 0;
    v->frame_queue.head       = NULL;

    unlock(vbv);

    return;
}


u8* vbv_get_current_write_addr(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return NULL;
    }
    
    return v->write_addr;
}


u32 vbv_get_valid_data_size(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return 0;
    }
    
    return v->valid_size;
}

u32 vbv_get_valid_frame_num(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return 0;
    }
    
    return v->frame_fifo.frame_num;
}
    
u32 vbv_get_max_stream_frame_num(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;
    
    if (v == NULL)
    {
        return 0;
    }
    
    return v->frame_fifo.max_frame_num;
}


void vbv_set_parent(void* parent, Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;

    if (v != NULL)
    {
    	v->parent = parent;
    }

    return;
}


void*  vbv_get_parent(Handle vbv)
{
    vbv_t* v = (vbv_t*)vbv;

    if (v != NULL)
    	return v->parent;
    else
    	return NULL;
}


static void enqueue_to_head(stream_frame_t* stream, stream_queue_t* q)
{
    stream->next = q->head;
    q->head = stream;
    q->frame_num++;
    return;
}


static void enqueue_to_tail(stream_frame_t* stream, stream_queue_t* q)
{
    stream_frame_t* node;
    
    node = q->head;
    
    if(node == NULL)
    {
        q->head = stream;
        q->head->next = NULL;
        q->frame_num++;
        return;
    }
    else
    {
        while(node->next != NULL)
            node = node->next;
        
        node->next = stream;
        stream->next = NULL;
        q->frame_num++;
        return;
    }
}


static stream_frame_t* dequeue(stream_queue_t* q)
{
    stream_frame_t* node;
    
    if(q->frame_num == 0)
        return NULL;
    
    node = q->head;
    q->head = q->head->next;
    node->next = NULL;
    q->frame_num--;
    
    return node;
}


static s32 lock(vbv_t* vbv)
{
    if(semaphore_pend(vbv->mutex, VBV_MUTEX_WAIT_TIME) != 0)
        return -1;

    return 0;
}


static void unlock(vbv_t* vbv)
{
	semaphore_post(vbv->mutex);
    return;
}


static void flush_stream_frame(vstream_data_t* stream, Handle vbv)
{
    vbv_flush_stream_frame(stream, vbv);
    vbv_free_vbs_sem(vbv);

}


//****************************************************************************//
//************************ Instance of VBV Interface *************************//
//****************************************************************************//

IVBV_t IVBV =
{
    vbv_request_stream_frame,
    vbv_return_stream_frame,
    flush_stream_frame,
    vbv_get_base_addr,
    vbv_get_buffer_size
};

