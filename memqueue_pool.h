#ifndef __MEM_QUEUE_POOL_H
#define __MEM_QUEUE_POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "salt_ssl_list.h"
#include "memqueue.h"

#ifdef _cplusplus
extern "C" {
#endif

#include <pthread.h>

/* 不可自增 */
struct memqueue_pool {
	u32 len;    	//队列个数
	void *owner;	//标记池子owner
    #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spinlock_t lock;
    #else
    pthread_mutex_t lock;
    #endif
	struct hlist_head head;
};


int  memqueue_pool_init(struct memqueue_pool *pool, u32 count, u32 len, u32 reserved,
    analysefunc  analyse, void *pool_owner);
int  memqueue_pool_deinit(struct memqueue_pool *pool);

/*  if queue_owner if NULL, return a empty queue ; whether return a queue owned by owner */
struct memqueue *memqueue_pool_get_queue(struct memqueue_pool *pool, void *queue_owner);


#ifdef _cplusplus
}
#endif
#endif