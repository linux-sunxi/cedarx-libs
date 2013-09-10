#ifndef CEDARX_HARDWARE_H
#define CEDARX_HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int cedarx_hardware_init(int mode);
extern int cedarx_hardware_exit(int mode);
extern void cedarx_register_print(char *func, int line);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
