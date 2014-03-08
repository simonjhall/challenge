#ifndef BCMCOMP_ALLOC_H
#define BCMCOMP_ALLOC_H

#include "bcmcomp.h"

BCM_COMP_API void *bcmcomp_malloc_surface(uint32_t size);
BCM_COMP_API void bcmcomp_free_surface(void *ptr);

#endif
