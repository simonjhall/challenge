/*
 * cache_cache.h
 *
 *  Created on: 19 Mar 2014
 *      Author: simon
 */

#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include "khrn_client_cache.h"

#ifdef __cplusplus
extern "C"
{
#endif

void add_new_entry(KHRN_MAP_T *cache, int key);
void touch_entry(KHRN_MAP_T *cache, int key);
void increment_frame_count(void);

#ifdef __cplusplus
}
#endif

#endif /* CACHE_CACHE_H_ */
