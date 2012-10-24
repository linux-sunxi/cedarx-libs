

#include "fbm.h"
#include "vdecoder_config.h"
#include "vdecoder.h"
#include "awprintf.h"

static void fbm_enqueue(frame_info_t** pp_head, frame_info_t* p);
static void fbm_enqueue_to_head(frame_info_t ** pp_head, frame_info_t *p);
static frame_info_t* fbm_dequeue(frame_info_t** pp_head);

static s32  lock(fbm_t* fbm);
static void unlock(fbm_t* fbm);


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

#if 0
	LOGV("  empty queue:\n");

	p = fbm->empty_queue;

	while(p)
	{
		LOGV("    frame %d, status = %s.\n", p->picture.id, status_string[p->status]);
		p = p->next;
	}

	LOGV("\n");

	LOGV("  display queue:\n");
	p = fbm->display_queue;
	while(p)
	{
		LOGV("    frame %d, status = %s.\n", p->picture.id, status_string[p->status]);
		p = p->next;
	}

	LOGV("\n");
#endif
	LOGV("  total queue:\n");
	for(i=0; i<fbm->max_frame_num; i++)
	{
		LOGV("frame=%x,frame %d, status = %s.\n", &(fbm->frames[i].picture), fbm->frames[i].picture.id, status_string[fbm->frames[i].status]);
	}

	LOGV("\n");
}
#endif


static s32 fbm_alloc_frame_buffer(vpicture_t* picture)
{
    picture->y 	   = NULL;
    picture->u 	   = NULL;
    picture->v 	   = NULL;
    picture->alpha = NULL;

    picture->y2	   	 = NULL;
    picture->v2	  	 = NULL;
    picture->u2	  	 = NULL;
    picture->alpha2  = NULL;


	if(picture->size_y != 0)
 	{
 		picture->y = (u8*)mem_palloc(picture->size_y, 1024);
 		if(picture->y == NULL)
 			goto _exit;

 	}

 	if(picture->size_u != 0)
 	{
 		picture->u = (u8*)mem_palloc(picture->size_u, 1024);
 		if(picture->u == NULL)
 		{
 			goto _exit;
 		}
 	}

 	if(picture->size_v != 0)
 	{
 		picture->v = (u8*)mem_palloc(picture->size_v, 1024);
 		if(picture->v == NULL)
 		{
 			goto _exit;
 		}
 	}

 	if(picture->size_alpha != 0)
 	{
 		picture->alpha = (u8*)mem_palloc(picture->size_alpha, 1024);
 		if(picture->alpha == NULL)
 		{
 			goto _exit;
 		}
 	}
	return 0;

_exit:
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
	if(picture->alpha != NULL){
		mem_pfree(picture->alpha);
		picture->alpha = NULL;
	}
	return -1;
}

s32 fbm_free_redBlue_frame_buffer(Handle h)
{
    u32 i = 0;
    fbm_t* 		  fbm = NULL;
    vpicture_t*   picture = NULL;

    fbm = (fbm_t*)h;

    if (lock(fbm) != 0)
    {
        return -1;
    }
    for(i=0; i<fbm->max_frame_num; i++)
 	{
        picture = &fbm->frames[i].picture;
        if(picture->y2 != NULL)
	    {
 		    mem_pfree(picture->y2);
 		    picture->y2 = NULL;
 	    }

 	    if(picture->u2 != NULL)
	    {
 		    mem_pfree(picture->u2);
 		    picture->u2 = NULL;
 	    }

 	    if(picture->v2 != NULL)
	    {
 		    mem_pfree(picture->v2);
 		    picture->v2 = NULL;
 	    }

 	    if(picture->alpha2 != NULL)
	    {
 		    mem_pfree(picture->alpha2);
 		    picture->alpha2 = NULL;
 	    }
    }
   unlock(fbm);
   return 0;
}


s32 fbm_alloc_redBlue_frame_buffer(Handle h)
{
    s32     	  i = 0;
    fbm_t* 		  fbm = NULL;
    vpicture_t*   picture = NULL;

    fbm = (fbm_t*)h;

    if(fbm == NULL)
        return -1;

    if(lock(fbm) != 0)
    {
        return -1;
    }
    picture = &fbm->frames[0].picture;
    if((picture->y2!=NULL) && (picture->u2!=NULL)&&(picture->v2!=NULL))
    {
        unlock(fbm);
        return 0;
    }

    for(i=0; i<(s32)fbm->max_frame_num; i++)
    {
        picture = &fbm->frames[i].picture;

        if(picture->size_y2)
        {
 		    picture->y2 = (u8 *)mem_palloc(picture->size_y2, 1024);
 		    if(picture->y2 == NULL)
            {
 			   break;
 		    }

 	    }
 	    if(picture->size_u2)
        {
 		    picture->u2 = (u8 *)mem_palloc(picture->size_u2, 1024);
 		    if(picture->u2 == NULL)
            {
 			    break;
 		    }
 	    }
 	    if(picture->size_v2)
        {
 		    picture->v2 = (u8 *)mem_palloc(picture->size_v2, 1024);
 		    if(picture->v2 == NULL)
            {
 			    break;
 		    }
 	    }
 	    if(picture->size_alpha2)
        {
 		    picture->size_alpha2 = (u32)mem_palloc(picture->size_alpha2, 1024);
 		    if(picture->alpha2 == NULL)
            {
 			    break;
 		    }
 	    }
   }

   if(i <(s32)fbm->max_frame_num)
   {
        unlock(fbm);
        fbm_free_redBlue_frame_buffer(h);
        return -1;
   }
   unlock(fbm);
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

 	if(picture->y2 != NULL)
	{
 		mem_pfree(picture->y2);
 		picture->y2 = NULL;
 	}

 	if(picture->u2 != NULL)
	{
 		mem_pfree(picture->u2);
 		picture->u2 = NULL;
 	}

 	if(picture->v2 != NULL)
	{
 		mem_pfree(picture->v2);
 		picture->v2 = NULL;
 	}

 	if(picture->alpha2 != NULL)
	{
 		mem_pfree(picture->alpha2);
 		picture->alpha2 = NULL;
 	}
 	return 0;
}


Handle fbm_init_ex(u32 max_frame_num,
                u32 min_frame_num,
                u32 size_y[2],
                u32 size_u[2],
                u32 size_v[2],
                u32 size_alpha[2],
                _3d_mode_e out_3d_mode,
                pixel_format_e format,
                void* parent)

{
    s32     i;
    fbm_t*  fbm;
    video_decoder_t* decoder;

    if(max_frame_num < min_frame_num)
    {
        max_frame_num = min_frame_num;
    }
	if(min_frame_num > FBM_MAX_FRAME_NUM)
		return NULL;

	if(max_frame_num > FBM_MAX_FRAME_NUM)
		max_frame_num = FBM_MAX_FRAME_NUM;

	fbm = (fbm_t*) mem_alloc(sizeof(fbm_t));
	if(!fbm)
	{
        return NULL;
	}
	mem_set(fbm, 0, sizeof(fbm_t));
    libve_io_ctrl(LIBVE_COMMAND_GET_PARENT, (u32)&decoder, parent);
    if(decoder == NULL)
    {
        mem_free(fbm);
    }
    if(decoder->status == CEDARV_STATUS_PREVIEW)
    {
        max_frame_num = 1;
        min_frame_num = 1;
    }
    //* alloc memory frame buffer.
    for(i=0; i<(s32)max_frame_num; i++)
    {
    	fbm->frames[i].picture.id 		  = i;
    	fbm->frames[i].picture.size_y 	  = size_y[0];
    	fbm->frames[i].picture.size_u 	  = size_u[0];
    	fbm->frames[i].picture.size_v 	  = size_v[0];
    	fbm->frames[i].picture.size_alpha = size_alpha[0];

    	fbm->frames[i].picture.size_y2		= size_y[1];
    	fbm->frames[i].picture.size_u2		= size_u[1];
    	fbm->frames[i].picture.size_v2		= size_v[1];
    	fbm->frames[i].picture.size_alpha2	= size_alpha[1];

    	if(fbm_alloc_frame_buffer(&fbm->frames[i].picture) != 0)
    	{
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
    if(out_3d_mode == _3D_MODE_DOUBLE_STREAM)
    {
       if(fbm_alloc_redBlue_frame_buffer((Handle)fbm) < 0)
       {
            fbm_release((Handle)fbm, parent);
            return NULL;
       }
    }


    //* initialize empty frame queue semaphore.
    fbm->mutex = semaphore_create(1);
    if(fbm->mutex == NULL)
    {
        fbm_release((Handle)fbm, parent);
        return NULL;
    }

    //* put all frame to empty frame queue.
    for(i=0; i<(s32)fbm->max_frame_num; i++)
    {
        fbm_enqueue(&fbm->empty_queue, &fbm->frames[i]);
    }

    return (Handle)fbm;
}


void fbm_release(Handle h, void* parent)
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
    	if(fbm->is_preview_mode)
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
        frame->status = FS_EMPTY;
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


void fbm_vdecoder_return_frame(vpicture_t * frame, Handle h)
{
	u8 idx;
	fbm_t * fbm;
	frame_info_t *frame_info;

	fbm = (fbm_t *)h;
	if(fbm == NULL)
		return ;
	idx = fbm_pointer_to_index(frame, h);
	frame_info = &fbm->frames[idx];
	if(lock(fbm) != 0)
		return ;
	fbm_enqueue_to_head(&fbm->display_queue, frame_info);

	unlock(fbm);
	return ;
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

void fbm_enqueue_to_head(frame_info_t ** pp_head, frame_info_t *p)
{
	frame_info_t * cur;

	cur = *pp_head;
	*pp_head = p;
	p->next = cur;
	return ;
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

