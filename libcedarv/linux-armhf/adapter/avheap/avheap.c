/*
********************************************************************************
*                                   AV Engine
*
*              Copyright(C), 2012-2016, All Winner Technology Co., Ltd.
*						        All Rights Reserved
*
* File Name   : avheap.c
*
* Author      : XC
*
* Version     : 0.1
*
* Date        : 2012.04.03
*
* Description : This file implements the av heap library.
*
* Others      : None at present.
*
* History     :
*
*  <Author>        <time>       <version>      <description>
*
*     XC         2012.04.03        0.1        build this file
*
********************************************************************************
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <avheap.h>
#include <cedardev_api.h>

#define loge(...)
#define logw(...)

//*************************************************************************//
//* define chunk type to present a block of memory.
//*************************************************************************//
typedef struct CHUNK chunk_t;

struct CHUNK
{
    int      page_offset;
    int      page_num;
    int      is_free;
    chunk_t* prev;
    chunk_t* next;
};

static chunk_t* chunk_create(int page_offset, int page_num)
{
    chunk_t* chunk = (chunk_t*)malloc(sizeof(chunk_t));
    if(chunk != NULL)
    {
        chunk->page_offset = page_offset;
        chunk->page_num    = page_num;
        chunk->is_free     = 1;
        chunk->prev        = NULL;
        chunk->next        = NULL;
    }

    return chunk;
}

static void chunk_destroy(chunk_t* chunk)
{
    free(chunk);
    return;
}


//*************************************************************************//
//* define chunk list and list operations.
//*************************************************************************//
typedef struct CHUNK_LIST
{
    chunk_t* first;
    chunk_t* last;

}chunk_list_t;

static chunk_list_t* chunk_list_create(void)
{
    chunk_list_t* list = (chunk_list_t*)malloc(sizeof(chunk_list_t));
    if(list != NULL)
    {
        list->first = NULL;
        list->last  = NULL;
    }

    return list;
}

static int chunk_list_is_empty(chunk_list_t* list)
{
    if(list->first == NULL)
        return 1;
    else
        return 0;
}

static chunk_t* chunk_list_head(chunk_list_t* list)
{
    return list->first;
}

#if 0   //* this method is not used.
static chunk_t* chunk_list_tail(chunk_list_t* list)
{
    return list->last;
}
#endif

static void chunk_list_insert_after(chunk_list_t* list, chunk_t* chunk, chunk_t* new_chunk)
{
    new_chunk->prev = chunk;
    new_chunk->next = chunk->next;

    if(chunk->next == NULL)
        list->last = new_chunk;
    else
        chunk->next->prev = new_chunk;

    chunk->next = new_chunk;

    return;
}

#if 0   //* this method is not used.
static void chunk_list_insert_before(chunk_list_t* list, chunk_t* chunk, chunk_t* new_chunk)
{
    new_chunk->prev = chunk->prev;
    new_chunk->next = chunk;

    if(chunk->prev == NULL)
        list->first = new_chunk;
    else
        chunk->prev->next = new_chunk;

    chunk->prev = new_chunk;

    return;
}
#endif

static void chunk_list_insert_head(chunk_list_t* list, chunk_t* new_chunk)
{
    if(list->first == NULL)
    {
        list->first = list->last = new_chunk;
        new_chunk->prev = new_chunk->next = NULL;
    }
    else
    {
        new_chunk->prev     = NULL;
        new_chunk->next     = list->first;
        list->first->prev   = new_chunk;
        list->first         = new_chunk;
    }

    return;
}

#if 0   //* this method is not used.
static void chunk_list_insert_tail(chunk_list_t* list, chunk_t* new_chunk)
{
    if(list->last == NULL)
        chunk_list_insert_head(list, new_chunk);
    else
    {
        new_chunk->prev = list->last;
        new_chunk->next = NULL;
        list->last->next = new_chunk;
        list->last = new_chunk;
    }

    return;
}
#endif

static chunk_t* chunk_list_remove(chunk_list_t* list, chunk_t* chunk)
{
    if(chunk->prev == NULL)
        list->first = chunk->next;
    else
        chunk->prev->next = chunk->next;

    if(chunk->next == NULL)
        list->last = chunk->prev;
    else
        chunk->next->prev = chunk->prev;

    return chunk;
}

static void chunk_list_destroy(chunk_list_t* list)
{
    chunk_t* chunk;
    while(chunk_list_is_empty(list) == 1)
    {
        chunk = chunk_list_head(list);
        chunk = chunk_list_remove(list, chunk);

        chunk_destroy(chunk);
    }

    free(list);
}



//*************************************************************************//
//* define heap allocator.
//*************************************************************************//

#define AV_PAGE_SIZE 4096

typedef struct ALLOCATOR
{
    chunk_list_t*       list;
    int                 heap_size;
}allocator_t;

static allocator_t* allocator_create(int heap_size)
{
    allocator_t* allocator;
    chunk_t*     chunk;

    allocator = (allocator_t*)malloc(sizeof(allocator_t));
    if(allocator == NULL)
        return NULL;

    allocator->list = chunk_list_create();
    if(allocator->list == NULL)
    {
        loge("can not create chunk list for allocator, return fail.");

        free(allocator);

        return NULL;
    }

    allocator->heap_size = heap_size;

    chunk = chunk_create(0, heap_size/AV_PAGE_SIZE);
    if(chunk == NULL)
    {
        loge("can not create a chunk when creating allocator, return fail.");

        chunk_list_destroy(allocator->list);
        free(allocator);

        return NULL;
    }

    chunk_list_insert_head(allocator->list, chunk);

    return allocator;
}


static void allocator_destroy(allocator_t* allocator)
{
    if(allocator != NULL)
    {
        chunk_list_destroy(allocator->list);

        free(allocator);
    }

    return;
}


static int allocator_alloc(allocator_t* allocator, int size)
{
    chunk_t* chunk_free;
    chunk_t* chunk_cur;
    chunk_t* chunk_split;
    int      free_page_num;
    int      page_num;
    int      left_page_num;

    int      heap_offset;

    if(size == 0)
        return -1;

    size     = size + (AV_PAGE_SIZE - 1);
    page_num = size / AV_PAGE_SIZE;

    chunk_free = NULL;
    chunk_cur  = chunk_list_head(allocator->list);

    while(chunk_cur != NULL)
    {
        //* search the list to find a best chunk.
        if(chunk_cur->is_free && chunk_cur->page_num >= page_num)
        {
            if(chunk_free == NULL || chunk_cur->page_num < chunk_free->page_num)
                chunk_free = chunk_cur;

            if(chunk_cur->page_num == page_num)
                break;
        }

        chunk_cur = chunk_cur->next;
    }

    if(chunk_free != NULL)
    {
        free_page_num = chunk_free->page_num;

        chunk_free->is_free  = 0;
        chunk_free->page_num = page_num;

        if(free_page_num > page_num)
        {
            left_page_num = free_page_num - page_num;

            chunk_split = chunk_create(chunk_free->page_offset + page_num, left_page_num);

            chunk_list_insert_after(allocator->list, chunk_free, chunk_split);
        }
    }

    if(chunk_free != NULL)
        heap_offset = chunk_free->page_offset * AV_PAGE_SIZE;
    else
        heap_offset = -1;

    return heap_offset;
}


static void allocator_free(allocator_t* allocator, int heap_offset)
{
    chunk_t* chunk_cur;
    chunk_t* chunk_prev;
    chunk_t* chunk_next;
    int      page_offset;

    if(allocator == NULL)
        return;

    page_offset = heap_offset / AV_PAGE_SIZE;
    chunk_cur = chunk_list_head(allocator->list);

    while(chunk_cur != NULL)
    {
        if(chunk_cur->page_offset == page_offset)
        {
            if(chunk_cur->is_free)
            {
                logw("chunk is already free.");
            }

            chunk_cur->is_free = 1;

            do
            {
                chunk_prev = chunk_cur->prev;
                chunk_next = chunk_cur->next;

                if(chunk_prev != NULL && (chunk_prev->is_free == 1 || chunk_prev->page_num == 0))
                {
                    chunk_prev->page_num += chunk_cur->page_num;

                    chunk_list_remove(allocator->list, chunk_cur);
                    chunk_destroy(chunk_cur);
                }

                chunk_cur = chunk_next;

            }while(chunk_cur != NULL && chunk_cur->is_free == 1);

            break;
        }

        chunk_cur = chunk_cur->next;
    }

    return;
}



//*************************************************************************//
//* define heap context.
//*************************************************************************//

#define MAX_BLOCK_NUM 1024

typedef struct BLOCK_INFO
{
    void* vaddr;        //* virtual memory address.

    void* paddr;        //* phisical memory address.

    int   is_used;      //* is this block info is used.

    int   size;         //* memory size of this block.

}block_info_t;

typedef struct HEAP_CONTEXT
{
    int                 fd;                           //* file descriptor of the ve driver.

    cedarv_env_info_t   env_info;                     //* information of memory provided by the ve driver.

    allocator_t*        allocator;                    //* heap allocator.

    pthread_mutex_t     mutex;

    block_info_t        block_info[MAX_BLOCK_NUM];    //* block information.

}heap_ctx_t;


static heap_ctx_t* heap_ctx = NULL;

//*************************************************************************//
//* get a not used block.
//*************************************************************************//
static int av_heap_get_a_free_block(heap_ctx_t* ctx)
{
    int i;

    for(i=0; i<MAX_BLOCK_NUM; i++)
    {
        if(ctx->block_info[i].is_used == 0)
            break;
    }

    if(i == MAX_BLOCK_NUM)
        return -1;
    else
        return i;
}

//*************************************************************************//
//* find a block with its virtual address.
//*************************************************************************//
static int av_heap_find_block_by_vaddr(heap_ctx_t* ctx, void* vaddr)
{
    int i;

    for(i=0; i<MAX_BLOCK_NUM; i++)
    {
        if(ctx->block_info[i].vaddr == vaddr)
            break;
    }

    if(i == MAX_BLOCK_NUM)
        return -1;
    else
        return i;
}


//*************************************************************************//
//* initialize the av heap.
//*************************************************************************//
int av_heap_init(int fd_ve)
{
    //* 1. allocate a heap context.
    if(heap_ctx != NULL)
    {
        loge("av heap context already initialized, return fail.");
        return -1;
    }

    heap_ctx = (heap_ctx_t*)malloc(sizeof(heap_ctx_t));
    if(heap_ctx == NULL)
    {
        loge("can not malloc memory for av heap context, return fail.");
        return -1;
    }

    memset(heap_ctx, 0, sizeof(heap_ctx_t));

    //* 2. initialize the mutex.
    if(pthread_mutex_init(&heap_ctx->mutex, NULL) != 0)
    {
        loge("can not initialize mutex for the av heap context, return fail.");

        free(heap_ctx);
        heap_ctx = NULL;

        return -1;
    }

    //* 3. open the driver.
    heap_ctx->fd = fd_ve;
    if(heap_ctx->fd == -1 || heap_ctx->fd == 0)
    {
        loge("file descriptor of driver '/dev/cedar_dev' is invalid, return fail.");

        pthread_mutex_destroy(&heap_ctx->mutex);
        free(heap_ctx);
        heap_ctx = NULL;

        return -1;
    }

    //* 4. get memory information.
    ioctl(heap_ctx->fd, IOCTL_GET_ENV_INFO, (unsigned int)&heap_ctx->env_info);

    //* 5. create a allocator.
    heap_ctx->allocator = allocator_create(heap_ctx->env_info.phymem_total_size);
    if(heap_ctx->allocator == NULL)
    {
        loge("can not create allocator for the heap context, return fail.");

        pthread_mutex_destroy(&heap_ctx->mutex);
        free(heap_ctx);
        heap_ctx = NULL;

        return -1;
    }

    return 0;
}


//*************************************************************************//
//* release the av heap.
//*************************************************************************//
void av_heap_release(void)
{
    if(heap_ctx == NULL)
    {
        loge("av heap context is not initialized yet.");
        return;
    }

    if(heap_ctx->allocator != NULL)
        allocator_destroy(heap_ctx->allocator);

    pthread_mutex_destroy(&heap_ctx->mutex);

    free(heap_ctx);

    heap_ctx = NULL;

    return;
}


//*************************************************************************//
//* allocate memory from the av heap.
//*************************************************************************//
void* av_heap_alloc(int size)
{
    int 		 offset;
    int 		 block_idx;
    unsigned int paddr;

    if(size <= 0)
    {
        loge("request memory of invalid size %d.", size);
        return NULL;
    }

    if(heap_ctx == NULL)
    {
        loge("av heap not initialized yet, return fail.");
        return NULL;
    }

    pthread_mutex_lock(&heap_ctx->mutex);

    offset = allocator_alloc(heap_ctx->allocator, size);
    if(offset < 0)
    {
        loge("av heap out of memory.");
        pthread_mutex_unlock(&heap_ctx->mutex);

        return NULL;
    }

    paddr = offset + heap_ctx->env_info.phymem_start;

    block_idx = av_heap_get_a_free_block(heap_ctx);
    if(block_idx < 0)
    {
        loge("av heap can not get a free block, return fail.");

        allocator_free(heap_ctx->allocator, offset);
        pthread_mutex_unlock(&heap_ctx->mutex);

        return NULL;
    }

    heap_ctx->block_info[block_idx].vaddr   = (void*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, heap_ctx->fd, paddr);
    heap_ctx->block_info[block_idx].paddr   = (void*)(paddr & 0x3fffffff);
    heap_ctx->block_info[block_idx].size    = size;
    heap_ctx->block_info[block_idx].is_used = 1;

    pthread_mutex_unlock(&heap_ctx->mutex);

    return heap_ctx->block_info[block_idx].vaddr;
}


//************************************************************************//
//* free memory allocated from the av heap.
//************************************************************************//
void av_heap_free(void* vaddr)
{
    int            block_idx;
    unsigned char* paddr;
    int            offset;

    if(heap_ctx == NULL)
    {
        loge("av heap not initialized yet.");
        return;
    }

    pthread_mutex_lock(&heap_ctx->mutex);

    block_idx = av_heap_find_block_by_vaddr(heap_ctx, vaddr);
    if(block_idx < 0)
    {
        loge("can not find a block of specific virtual address %x.", vaddr);
        pthread_mutex_unlock(&heap_ctx->mutex);
        return;
    }

    paddr  = (unsigned char*)heap_ctx->block_info[block_idx].paddr;
    offset = (int)(paddr - (heap_ctx->env_info.phymem_start & 0x3fffffff));

    munmap(heap_ctx->block_info[block_idx].vaddr, heap_ctx->block_info[block_idx].size);

    allocator_free(heap_ctx->allocator, offset);

    memset(&heap_ctx->block_info[block_idx], 0, sizeof(block_info_t));

    pthread_mutex_unlock(&heap_ctx->mutex);

    return;
}


//************************************************************************//
//* get physical address from a virtual memory address.
//************************************************************************//
void* av_heap_physic_addr(void* vaddr)
{
    int          block_idx;
    unsigned int block_start;
    unsigned int block_end;
    unsigned int cur_addr;

    if(heap_ctx == NULL)
    {
        loge("av heap not initialized yet.");
        return NULL;
    }

    if(vaddr == NULL)
        return NULL;

    pthread_mutex_lock(&heap_ctx->mutex);

    cur_addr = (unsigned int)vaddr;

    for(block_idx = 0; block_idx < MAX_BLOCK_NUM; block_idx++)
    {
        if(heap_ctx->block_info[block_idx].is_used == 1)
        {
            block_start = (unsigned int)heap_ctx->block_info[block_idx].vaddr;
            block_end   = block_start + heap_ctx->block_info[block_idx].size;

            if(block_start <= cur_addr && cur_addr < block_end)
                break;
        }
    }

    pthread_mutex_unlock(&heap_ctx->mutex);

    if(block_idx == MAX_BLOCK_NUM)
        return NULL;

    return (unsigned char*)heap_ctx->block_info[block_idx].paddr + (cur_addr - block_start);
}


