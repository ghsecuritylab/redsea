/*
 * Copyright 2015 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_MALLOC_H__
#define __AYLA_MALLOC_H__

#ifdef AMEBA

#define calloc ayla_calloc

void *ayla_calloc(size_t count, size_t size);
#define ayla_free vPortFree

#if 1//add by will
#undef malloc
#undef free
#define malloc (void *)pvPortMalloc
#define free vPortFree
#endif

#elif defined(WMSDK)
#include <wm_os.h>

#define malloc os_mem_alloc
#define realloc os_mem_realloc
#define free os_mem_free
#define calloc ayla_calloc

#if 0
static inline void *ayla_calloc(size_t count, size_t size)
{
	unsigned char *p;

	PRINTF("\n\r[cs] 2 available heap %d\n\r", xPortGetFreeHeapSize());

	p = os_mem_calloc(count * size);
	if (p == NULL){
		printf("[cs] malloc failed!\r\n");
	}
	return (void *)p;
}
#endif // 0

#endif /* WMSDK */

#endif /* __AYLA_MALLOC_H__ */
