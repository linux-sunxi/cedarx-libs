
#ifdef MELIS
#include "ePDK.h"
#else
#include <stdarg.h>
#endif

#define PRINT_MODE_NOP				0
#define PRINT_MODE_SAVE_TO_FILE		1
#define PRINT_MODE_USE_ANDROID_LOG	2
#define PRINT_MODE_DIRECT			3

#define PRINT_MODE					PRINT_MODE_NOP

#if (PRINT_MODE == PRINT_MODE_SAVE_TO_FILE)
#ifdef MELIS
	#define PRINT_FILE_PATH			"e:\\vdecoder_print.txt"
#else
	#define PRINT_FILE_PATH			"/flash/vdecoder_print.txt"
#endif
	#define LOG_BUFFER_SIZE			1024*512
#endif

int  awprintf_init(void);
void awprintf_exit(void);
int  awprintf(const char* func, int line, ...);
int  awvprintf(const char* func, int line, va_list args);

#if (PRINT_MODE == PRINT_MODE_NOP)

#define 	AwLogOpen()
#define		AwLogClose()
#define 	AwLog(...)
#define		AwVLog(...)
#define		AwLogX(...)
#define 	AwVLogX(...)

#else

#define 	AwLogOpen()							awprintf_init()
#define		AwLogClose()						awprintf_exit()
#define 	AwLog(...)							awprintf(__func__, __LINE__, __VA_ARGS__)
#define		AwVLog(func, line, args)			awvprintf(func, line, args)

#define     AwLogX(switch, ...)										\
				do													\
				{													\
					if (switch == 1)								\
						awprintf(__func__, __LINE__, __VA_ARGS__);	\
				}while(0)

#define     AwVLogX(switch, func, line, args)						\
				do													\
				{													\
					if (switch == 1)								\
						awvprintf(func, line, args);				\
				}while(0)

#endif

    #define LOGLEVEL0					(0)
    #define LOGLEVEL1					(0)
    #define LOGLEVEL2					(0)

    #define L0							LOGLEVEL0
    #define L1							LOGLEVEL1
    #define L2							LOGLEVEL2

    #define LogOpen						AwLogOpen
    #define LogClose					AwLogClose
	#define Log							AwLog
	#define LogX						AwLogX
	#define vLog						AwVLog
	#define vLogX						AwVLogX

