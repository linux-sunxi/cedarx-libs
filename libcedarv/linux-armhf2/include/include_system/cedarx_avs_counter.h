#ifndef CEDARX_AVS_COUNTER_H
#define CEDARX_AVS_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <pthread.h>
typedef struct CedarxAvscounterContext {
	long long sample_time;
	long long base_time;
	int adjust_ratio;
	long long system_base_time;

	pthread_mutex_t mutex;

	void (*reset)(struct CedarxAvscounterContext *context);
	void (*get_time)(struct CedarxAvscounterContext *context, long long *curr);
	void (*get_time_diff)(struct CedarxAvscounterContext *context, long long *diff);
	void (*adjust)(struct CedarxAvscounterContext *context, int val);
}CedarxAvscounterContext;

CedarxAvscounterContext *cedarx_avs_counter_request();
int cedarx_avs_counter_release(CedarxAvscounterContext *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
