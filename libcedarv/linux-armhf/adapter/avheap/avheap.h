/*
********************************************************************************
*                                   AV Engine
*
*              Copyright(C), 2012-2016, All Winner Technology Co., Ltd.
*						        All Rights Reserved
*
* File Name   : avheap.h
*
* Author      : XC
*
* Version     : 0.1
*
* Date        : 2012.04.03
*
* Description : This is the header file of the av heap library.
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

#ifndef AVHEAP_H
#define AVHEAP_H

//*************************************************************************//
//* initialize the av heap.
//*************************************************************************//
int av_heap_init(int fd_ve);

//*************************************************************************//
//* release the av heap.
//*************************************************************************//
void av_heap_release(void);

//*************************************************************************//
//* allocate memory from the av heap.
//*************************************************************************//
void* av_heap_alloc(int size);

//************************************************************************//
//* free memory allocated from the av heap.
//************************************************************************//
void av_heap_free(void* mem);

//************************************************************************//
//* get physical address from a virtual memory address.
//************************************************************************//
void* av_heap_physic_addr(void* vaddr);


#endif

