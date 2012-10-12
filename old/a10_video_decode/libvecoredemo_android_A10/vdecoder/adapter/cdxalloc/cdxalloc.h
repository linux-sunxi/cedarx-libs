
#ifndef CDX_ALLOC_H
#define CDX_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

	int 		 cdxalloc_open(void);
	int 		 cdxalloc_close(void);
	void* 		 cdxalloc_alloc(int size);
	void 		 cdxalloc_free(void* address);
	unsigned int cdxalloc_vir2phy(void* address);


#ifdef __cplusplus
}
#endif

#endif

