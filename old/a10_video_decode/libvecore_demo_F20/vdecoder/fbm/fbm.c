
#include "fbm.h"
#include "vdecoder_config.h"
#include "vdecoder.h"

static void fbm_enqueue(frame_info_t** pp_head, frame_info_t* p);
static frame_info_t* fbm_dequeue(frame_info_t** pp_head);

static s32  lock(fbm_t* fbm);
static void unlock(fbm_t* fbm);

extern video_decoder_t* g_vdecoder;

#if 0
void print_fbm_status(Handle h)
{
	const u8* status_string[5] =
	{
		"FS_EMPTY",
		"FS_DECODER_USING",
		"FS_DISPLAY_USING",
		"FS_DECODER_AND_DISPLAY_USING",
		"FS_DECODER_USING_DISPLAY_DISCARD",
	};

	fbm_t*        fbm;
	frame_info_t* p;
	u32			  i;

	fbm = (fbm_t*)h;

	sys_print("  empty queue:\n");

	p = fbm->empty_queue;
	while(p)
	{
		sys_print("    frame %d, status = %s.\n", p->picture.id, status_string[p->status]);
		p = p->next;
	}

	sys_print("\n");

	sys_print("  display queue:\n");
	p = fbm->display_queue;
	while(p)
	{
		sys_print("    frame %d, status = %s.\n", p->picture.id, status_string[p->status]);
		p = p->next;
	}

	sys_print("\n");

	sys_print("  total queue:\n");
	for(i=0; i<fbm->max_frame_num; i++)
	{
		sys_print("    frame %d, status = %s.\n", fbm->frames[i].picture.id, status_string[fbm->frames[i].status]);
	}

	sys_print("\n");
}
#endif


static s32 fbm_alloc_frame_buffer(vpicture_t* picture)
{
    picture->y 	   = NULL;
    picture->u 	   = NULL;
    picture->v 	   = NULL;
    picture->alpha = NULL;

	if(picture->size_y != 0)
 	{
 		picture->y = (u8*)mem_palloc(picture->size_y, 1024);
 		if(picture->y == NULL)
 			return -1;
 	}

 	if(picture->size_u != 0)
 	{
 		picture->u = (u8*)mem_palloc(picture->size_u, 1024);
 		if(picture->u == NULL)
 		{
 			if(picture->y != NULL)
 			{
 				mem_pfree(picture->y);
 				picture->y = NULL;
 			}
 			return -1;
 		}
 	}

 	if(picture->size_v != 0)
 	{
 		picture->v = (u8*)mem_palloc(picture->size_v, 1024);
 		if(picture->v == NULL)
 		{
 			if(picture->u != NULL)
 			{
 				mem_pfree(picture->u);
 				picture->u = NULL;
 			}

 			if(picture->y != NULL)
 			{
 				mem_pfree(picture->y);
 				picture->y = NULL;
 			}
 			return -1;
 		}
 	}

 	if(picture->size_alpha != 0)
 	{
 		picture->alpha = (u8*)mem_palloc(picture->size_alpha, 1024);
 		if(picture->alpha == NULL)
 		{
 			if(picture->v != NULL)
 			{
 				mem_pfree(picture->v);
 				picture->v = NULL;
 			}

 			if(picture->u != NULL)
 			{
 				mem_pfree(picture->u);
 				picture->u = NULL;
 			}

 			if(picture->y != NULL)
 			{
 				mem_pfree(picture->y);
 				picture->y = NULL;
 			}

 			return -1;
 		}
 	}

	return 0;
}

static s32 fbm_free_frame_buffer(vpicture_t* picture)
{
 	if(picture->y != NULL)
 	{
 		mem_pfree(picture->y);
 		picture->y = NULL;
 	}

 	if(picture->u != NULL)
 	{
 		mem_pfree(picture->u);
 		picture->u = NULL;
 	}

 	if(picture->v != NULL)
 	{
 		mem_pfree(picture->v);
 		picture->v = NULL;
 	}

 	if(picture->alpha != NULL)
 	{
 		mem_pfree(picture->alpha);
 		picture->alpha = NULL;
	}

	return 0;
}


Handle fbm_init(u32 max_frame_num,
                u32 min_frame_num,
                u32 size_y,
                u32 size_u,
                u32 size_v,
                u32 size_alpha,
                pixel_format_e format)
{
    s32     i;
    fbm_t*  fbm;

	if(max_frame_num < min_frame_num)
		return NULL;

	if(min_frame_num > FBM_MAX_FRAME_NUM)
		return NULL;

	if(max_frame_num > FBM_MAX_FRAME_NUM)
		max_frame_num = FBM_MAX_FRAME_NUM;

	fbm = (fbm_t*) mem_alloc(sizeof(fbm_t));
	if(!fbm)
		return NULL;
	mem_set(fbm, 0, sizeof(fbm_t));

    //* alloc memory frame buffer.
    for (i=0; i<(s32)max_frame_num; i++)
    {
    	fbm->frames[i].picture.id 		  = i;
    	fbm->frames[i].picture.size_y 	  = size_y;
    	fbm->frames[i].picture.size_u 	  = size_u;
    	fbm->frames[i].picture.size_v 	  = size_v;
    	fbm->frames[i].picture.size_alpha = size_alpha;

    	if(g_vdecoder != NULL && g_vdecoder->status == CEDARV_STATUS_PREVIEW)
    	{
    		//* in preview mode, we only use one frame buffer.
    		if(i==0)
    		{
    			if(fbm_alloc_frame_buffer(&fbm->frames[i].picture) != 0)
    				break;
    		}
    		else
    		{
    			fbm->frames[i].picture.y 	 = fbm->frames[0].picture.y;
    			fbm->frames[i].picture.u 	 = fbm->frames[0].picture.u;
    			fbm->frames[i].picture.v 	 = fbm->frames[0].picture.v;
    			fbm->frames[i].picture.alpha = fbm->frames[0].picture.alpha;
    		}
    	}
    	else
    	{
    		if(fbm_alloc_frame_buffer(&fbm->frames[i].picture) != 0)
    			break;
    	}
    }

    if(i < (s32)min_frame_num)
    {
    	for(; i>=0; i--)
    		fbm_free_frame_buffer(&fbm->frames[i].picture);

    	mem_free(fbm);
    	return NULL;
    }

    fbm->max_frame_num = i;

    //* initialize empty frame queue semaphore.
    fbm->mutex = semaphore_create(1);
    if(fbm->mutex == NULL)
    {
        fbm_release((Handle)fbm);
        return NULL;
    }

    //* put all frame to empty frame queue.
    for(i=0; i<(s32)fbm->max_frame_num; i++)
    {
        fbm_enqueue(&fbm->empty_queue, &fbm->frames[i]);
    }

    return (Handle)fbm;
}


void fbm_release(Handle h)
{
    u32     i;
    fbm_t*  fbm;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return;

    if(fbm->mutex)
    {
    	semaphore_delete(fbm->mutex, SEM_DEL_ALWAYS);
        fbm->mutex = NULL;
    }

    for(i=0; i<fbm->max_frame_num; i++)
    {
    	if(g_vdecoder != NULL && g_vdecoder->status == CEDARV_STATUS_PREVIEW)
    	{
    		if(i == 0)
    		{
    			fbm_free_frame_buffer(&fbm->frames[i].picture);
    			break;
    		}
    	}
    	else
    	{
    		fbm_free_frame_buffer(&fbm->frames[i].picture);
    	}
    }

    mem_free(fbm);

    return;
}


s32 fbm_reset(u8 reserved_frame_index, Handle h)
{
    u8     		  i;
    u8     		  flag;
    fbm_t* 		  fbm;
    frame_info_t* ptr;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return -1;

    if (lock(fbm) != 0)
    {
        return -1;
    }

    flag = 0;
    if(reserved_frame_index < fbm->max_frame_num)
    {
    	ptr = fbm->empty_queue;
    	while(ptr != NULL)
    	{
    		if(ptr->picture.id == reserved_frame_index)
    		{
    			flag = 1;
    			break;
    		}

    		ptr = ptr->next;
    	}

    	ptr = fbm->display_queue;
    	while(ptr != NULL)
    	{
    		if(ptr->picture.id == reserved_frame_index)
    		{
    			flag = 2;
    			break;
    		}

    		ptr = ptr->next;
    	}
    }

    fbm->empty_queue   = NULL;
    fbm->display_queue = NULL;

    for (i=0; i<fbm->max_frame_num; i++)
    {
        if (i == reserved_frame_index)
            continue;

        fbm->frames[i].next   = NULL;
        fbm->frames[i].status = FS_EMPTY;
    }

    for (i=0; i<fbm->max_frame_num; i++)
    {
    	if(i == reserved_frame_index)
    	{
    		if(flag == 1)
    			fbm_enqueue(&fbm->empty_queue, &fbm->frames[reserved_frame_index]);
    		else if (flag == 2)
    			fbm_enqueue(&fbm->display_queue, &fbm->frames[reserved_frame_index]);
    	}
    	else
            fbm_enqueue(&fbm->empty_queue, &fbm->frames[i]);
    }

    unlock(fbm);

    return 0;
}


s32 fbm_flush(Handle h)
{
    fbm_t* 		  fbm;
    frame_info_t* frame;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return -1;

    if (lock(fbm) != 0)
    {
        return -1;
    }

	while((frame = fbm_dequeue(&fbm->display_queue)) != NULL)
	{
		fbm_enqueue(&fbm->empty_queue, frame);
	}

    unlock(fbm);

    return 0;
}


vpicture_t* fbm_decoder_request_frame(Handle h)
{
	u32           i;
    fbm_t*        fbm;
    frame_info_t* frame_info;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return NULL;

    frame_info = NULL;

    for(i=0; i<FBM_REQUEST_FRAME_WAIT_TIME/10; i++)
    {
    	lock(fbm);

    	frame_info = fbm_dequeue(&fbm->empty_queue);
    	if(frame_info != NULL)
    	{
        	frame_info->status = FS_DECODER_USING;
    	}

    	unlock(fbm);

    	if(frame_info != NULL)
    		break;
    	else
    		sys_sleep(10);
    }

    if(frame_info != NULL)
    {
        return &frame_info->picture;
    }
    else
        return NULL;
}


void fbm_decoder_return_frame(vpicture_t* frame, u8 valid, Handle h)
{
    u8            idx;
    fbm_t*        fbm;
    frame_info_t* frame_info;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return;

    idx = fbm_pointer_to_index(frame, h);
    if(idx >= fbm->max_frame_num)
        return;

    frame_info = &fbm->frames[idx];

    if(lock(fbm) != 0)
        return;

    if(frame_info->status == FS_DECODER_USING)
    {
        if(valid)
        {
            frame_info->status = FS_DISPLAY_USING;
            fbm_enqueue(&fbm->display_queue, frame_info);
        }
        else
        {
            frame_info->status = FS_EMPTY;
            fbm_enqueue(&fbm->empty_queue, frame_info);
        }
    }
    else if(frame_info->status == FS_DECODER_AND_DISPLAY_USING)
    {
        frame_info->status = FS_DISPLAY_USING;
    }
    else if(frame_info->status == FS_DECODER_USING_DISPLAY_DISCARD)
    {
        frame_info->status = FS_EMPTY;
        fbm_enqueue(&fbm->empty_queue, frame_info);
    }
    else
    {
        //* error case, program should not run to here.
    }

    unlock(fbm);

    return;
}


void fbm_decoder_share_frame(vpicture_t* frame, Handle h)
{
    u8            idx;
    fbm_t*        fbm;
    frame_info_t* frame_info;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return;

    idx = fbm_pointer_to_index(frame, h);
    if(idx >= fbm->max_frame_num)
        return;

    frame_info = &fbm->frames[idx];

    if(lock(fbm) != 0)
        return;

    frame_info->status = FS_DECODER_AND_DISPLAY_USING;
    fbm_enqueue(&fbm->display_queue, frame_info);

    unlock(fbm);

    return;
}


vpicture_t* fbm_display_request_frame(Handle h)
{
    fbm_t*        fbm;
    frame_info_t* frame_info;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return NULL;

    frame_info = NULL;

    if(lock(fbm) != 0)
        return NULL;

    frame_info = fbm_dequeue(&fbm->display_queue);

    unlock(fbm);

    if(frame_info != NULL)
        return &frame_info->picture;
    else
        return NULL;
}


void fbm_display_return_frame(vpicture_t* frame, Handle h)
{
    u8            idx;
    fbm_t*        fbm;
    frame_info_t* frame_info;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return;

    idx = fbm_pointer_to_index(frame, h);
    if(idx >= fbm->max_frame_num)
        return;

    frame_info = &fbm->frames[idx];

    if(lock(fbm) != 0)
        return;

    if(frame_info->status == FS_DISPLAY_USING)
    {
        frame_info->status = FS_EMPTY;
        fbm_enqueue(&fbm->empty_queue, frame_info);
    }
    else if(frame_info->status == FS_DECODER_AND_DISPLAY_USING)
    {
        frame_info->status = FS_DECODER_USING_DISPLAY_DISCARD;
    }
    else
    {
        //* error case, program should not run to here.
    }

    unlock(fbm);

    return;
}


vpicture_t* fbm_display_pick_frame(Handle h)
{
    fbm_t*        fbm;
    frame_info_t* frame_info;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return NULL;

    frame_info = fbm->display_queue;

    if(frame_info != NULL)
        return &frame_info->picture;
    else
        return NULL;
}


vpicture_t* fbm_index_to_pointer(u8 index, Handle h)
{
    fbm_t*        fbm;
    frame_info_t* frame_info;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return NULL;

    if(index >= fbm->max_frame_num)
        return NULL;

    frame_info = &fbm->frames[index];
    return &frame_info->picture;
}


u8 fbm_pointer_to_index(vpicture_t* frame, Handle h)
{
    u8            i;
    fbm_t*        fbm;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return 0xff;

    for(i=0; i<fbm->max_frame_num; i++)
    {
        if(&fbm->frames[i].picture == frame)
            break;
    }

    return (i < fbm->max_frame_num) ? i : 0xff;
}


static void fbm_enqueue(frame_info_t** pp_head, frame_info_t* p)
{
    frame_info_t* cur;

    cur = *pp_head;

    if(cur == NULL)
    {
        *pp_head = p;
        p->next  = NULL;
        return;
    }
    else
    {
        while(cur->next != NULL)
            cur = cur->next;

        cur->next = p;
        p->next   = NULL;

        return;
    }
}


static frame_info_t* fbm_dequeue(frame_info_t** pp_head)
{
    frame_info_t* head;

    head = *pp_head;

    if(head == NULL)
        return NULL;
    else
    {
        *pp_head = head->next;
        head->next = NULL;
        return head;
    }
}

#if (1)
static s32 lock(fbm_t* fbm)
{
    if(semaphore_pend(fbm->mutex, FBM_MUTEX_WAIT_TIME) != 0)
    {
        return -1;
    }

    return 0;
}


static void unlock(fbm_t* fbm)
{
	semaphore_post(fbm->mutex);
    return;
}
#else

static s32 cpu_sr;
static s32 lock(fbm_t* fbm)
{
	ENTER_CRITICAL(cpu_sr);
	return 0;
}


static void unlock(fbm_t* fbm)
{
	EXIT_CRITICAL(cpu_sr);
}
#endif

