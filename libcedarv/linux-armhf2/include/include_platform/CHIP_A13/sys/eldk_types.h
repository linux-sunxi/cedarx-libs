#ifndef __ELDK_TYPES_H__
#define __ELDK_TYPES_H__

#ifndef __KERNEL__
#include <semaphore.h>
typedef sem_t  OS_SEM_DATA;
typedef pthread_mutex_t  __krnl_event_t;
#endif

typedef unsigned long long    __u64;
typedef unsigned int        __u32;
typedef unsigned short      __u16;
typedef unsigned char       __u8;
typedef signed long long      __s64;
typedef signed int          __s32;
typedef signed short        __s16;
typedef signed char         __s8;
typedef signed char         __bool;

//typedef unsigned long long    u64;
//typedef unsigned int        u32;
//typedef unsigned short      u16;
//typedef unsigned char       u8;
//typedef signed long long      s64;
//typedef signed int          s32;
//typedef signed short        s16;
//typedef signed char         s8;
//typedef signed char         bool;


#define EPDK_OK                  0
#define EPDK_FAIL               -1
#define EPDK_TRUE                1
#define EPDK_FALSE               0

#endif
