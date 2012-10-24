
#include "awprintf.h"
#include "os_adapter.h"

#if PRINT_MODE == PRINT_MODE_USE_ANDROID_LOG

#define LOG_NDEBUG 0
#define LOG_TAG ""
#include <CDX_Debug.h>

#endif

#ifdef MELIS
#include "epdk.h"
#else
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#endif


#if PRINT_MODE == PRINT_MODE_SAVE_TO_FILE


	static char* log_buffer  		= NULL;
	static int   log_cur_pos 		= 0;

#ifdef MELIS
	static ES_FILE* log				= NULL;
#else
	static FILE* log				= NULL;
#endif

#endif

	#define LOG_BUFFER_MAX_LINE_SIZE	1024
	static char* log_line_buffer = NULL;

int awprintf_init(void)
{
	log_line_buffer = (char*)mem_alloc(LOG_BUFFER_MAX_LINE_SIZE);
	if(log_line_buffer == NULL)
	{
		return -1;
	}

#if (PRINT_MODE == PRINT_MODE_SAVE_TO_FILE)

	log_buffer = (char*)mem_alloc(LOG_BUFFER_SIZE);

#ifdef MELIS
	log = eLIBs_fopen(PRINT_FILE_PATH, "wb");
#else
	log = fopen(PRINT_FILE_PATH, "wb");
#endif

	if(log_buffer == NULL || log == NULL)
	{
		if(log_buffer != NULL)
		{
			mem_free(log_buffer);
			log_buffer = NULL;
		}

		if(log_line_buffer != NULL)
		{
			mem_free(log_line_buffer);
			log_line_buffer = NULL;
		}

		if(log != NULL)
		{
#ifdef MELIS
			eLIBs_fclose(log);
#else
			fclose(log);
#endif
			log = NULL;
		}

		return -1;
	}

	log_cur_pos = 0;

#endif

	return 0;
}

void awprintf_exit(void)
{
	if(log_line_buffer != NULL)
	{
		mem_free(log_line_buffer);
		log_line_buffer = NULL;
	}

#if (PRINT_MODE == PRINT_MODE_SAVE_TO_FILE)

	if(log_cur_pos > 0)
	{
#ifdef MELIS
		eLIBs_fwrite(log_buffer, 1, log_cur_pos, log);
#else
		fwrite(log_buffer, 1, log_cur_pos, log);
#endif
		log_cur_pos = 0;
	}

	if(log_buffer != NULL)
	{
		mem_free(log_buffer);
		log_buffer = NULL;
	}

	if(log != NULL)
	{
#ifdef MELIS
		eLIBs_fclose(log);
#else
		fclose(log);
#endif
		log = NULL;
	}

	log_cur_pos = 0;
#endif

	return;
}


int awprintf(const char* func, int line, ...)
{
	char*   pos;
#if (PRINT_MODE == PRINT_MODE_SAVE_TO_FILE)
	char*   src;
	char*   dst;
#endif

	va_list args;
	char*   format;

#ifdef MELIS
	eLIBs_sprintf(log_line_buffer, "(f:%s, l:%d) ", func, line);
#else
	sprintf(log_line_buffer, "(f:%s, l:%d) ", func, line);
#endif
	pos = log_line_buffer;
	while(*pos)
		pos++;

	va_start(args, line);
	format = va_arg(args, char*);
#ifdef MELIS
    eLIBs_vsprintf(pos, format, args);
#else
    vsprintf(pos, format, args);
#endif
    va_end(args);

#if (PRINT_MODE == PRINT_MODE_USE_ANDROID_LOG)
	LOGV("%s", log_line_buffer);
#endif

#if (PRINT_MODE == PRINT_MODE_DIRECT)
	#ifdef MELIS
	eLIBs_printf("%s\n", log_line_buffer);
	#else
	printf("%s\n", log_line_buffer);
	#endif
#endif

#if (PRINT_MODE == PRINT_MODE_SAVE_TO_FILE)

	src = log_line_buffer;
	dst = log_buffer + log_cur_pos;

	while(*src)
	{
		*dst++ = *src++;

		log_cur_pos++;
		if(log_cur_pos >= LOG_BUFFER_SIZE)
		{
#ifdef MELIS
			eLIBs_fwrite(log_buffer, 1, log_cur_pos, log);
#else
			fwrite(log_buffer, 1, log_cur_pos, log);
#endif
			log_cur_pos = 0;
			dst = log_buffer;
		}
    }

	*dst++ = '\n';
	log_cur_pos++;
	if(log_cur_pos >= LOG_BUFFER_SIZE)
	{
#ifdef MELIS
		eLIBs_fwrite(log_buffer, 1, log_cur_pos, log);
#else
		fwrite(log_buffer, 1, log_cur_pos, log);
#endif
		log_cur_pos = 0;
		dst = log_buffer;
	}

#endif

	return 0;
}


int awvprintf(const char* func, int line, va_list args)
{
	char*   pos;
#if (PRINT_MODE == PRINT_MODE_SAVE_TO_FILE)
	char*   src;
	char*   dst;
#endif

	char*   format;

#ifdef MELIS
	eLIBs_sprintf(log_line_buffer, "(f:%s, l:%d) ", func, line);
#else
	sprintf(log_line_buffer, "(f:%s, l:%d) ", func, line);
#endif
	pos = log_line_buffer;
	while(*pos)
		pos++;

	format = va_arg(args, char*);
#ifdef MELIS
    eLIBs_vsprintf(pos, format, args);
#else
    vsprintf(pos, format, args);
#endif

#if (PRINT_MODE == PRINT_MODE_USE_ANDROID_LOG)
	LOGV("%s", log_line_buffer);
#endif

#if (PRINT_MODE == PRINT_MODE_DIRECT)
	#ifdef MELIS
	eLIBs_printf("%s\n", log_line_buffer);
	#else
	printf("%s\n", log_line_buffer);
	#endif
#endif

#if (PRINT_MODE == PRINT_MODE_SAVE_TO_FILE)

	src = log_line_buffer;
	dst = log_buffer + log_cur_pos;

	while(*src)
	{
		*dst++ = *src++;

		log_cur_pos++;
		if(log_cur_pos >= LOG_BUFFER_SIZE)
		{
#ifdef MELIS
			eLIBs_fwrite(log_buffer, 1, log_cur_pos, log);
#else
			fwrite(log_buffer, 1, log_cur_pos, log);
#endif
			log_cur_pos = 0;
			dst = log_buffer;
		}
    }

	*dst++ = '\n';
	log_cur_pos++;
	if(log_cur_pos >= LOG_BUFFER_SIZE)
	{
#ifdef MELIS
		eLIBs_fwrite(log_buffer, 1, log_cur_pos, log);
#else
		fwrite(log_buffer, 1, log_cur_pos, log);
#endif
		log_cur_pos = 0;
		dst = log_buffer;
	}

#endif

	return 0;
}

