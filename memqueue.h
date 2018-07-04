#ifndef __MEM_QUEUE_H
#define __MEM_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "salt_ssl_list.h"
#ifdef _cplusplus
extern "C" {
#endif
#include <pthread.h>
#include <time.h>


typedef unsigned long       u64;
typedef unsigned int        u32;
typedef int                 s32;
typedef unsigned short      u16;
typedef short               s16;
typedef unsigned char       u8;
typedef signed char         s8;

#define MEMQUEUE_SPIN_LOCK_DEFINE
#undef MEMQUEUE_SPIN_LOCK_DEFINE

struct memqueue;
typedef int (*analysefunc)(struct memqueue *q,char *data, u32 *len);

struct memqueue {
    char *head; //内存头部
    char *data; //真实数据头部
    char *tail; //内存尾部
    char *end;  //真实数据尾部
	void *owner;//标记队列owner
    u32 len;    //内存总长
    u32 reserved;
    
    #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spinlock_t lock;
    #else
    pthread_mutex_t lock;    
    pthread_cond_t  cond_push;
    pthread_cond_t  cond_pop;
    struct timespec wtime;
    #endif
    
    analysefunc  analyse;//注册解析函数，*len 返回需要Pop的长度
	struct hlist_node list;
};

int  memqueue_init(struct memqueue *q, u32 len, u32 reserved,  analysefunc  analyse);
int  memqueue_deinit(struct memqueue *q);


int  memqueue_push(struct memqueue *q, char *mem, u32 len);
int  memqueue_pop(struct memqueue *q, char *mem, u32 *len);

/* warn: enter memqueue data  */
char *memqueue_get_prepushbuff_continuely(struct memqueue *q, u32 len);
int  memqueue_get_prepushbuff_continuely_recorrect(struct memqueue *q, u32 len);
char *memqueue_get_prepopbuff_continuely(struct memqueue *q, u32 *len);
int  memqueue_get_prepopbuff_continuely_recorrect(struct memqueue *q);

int  memqueue_rst_data_end(struct memqueue *q);
int  memqueue_reserved(struct memqueue *q, u32 len);


int  memqueue_rst_analyse(struct memqueue *q, analysefunc  analyse);
int  memqueue_bind(struct memqueue *q, void *queue_owner);
void *memqueue_owner(struct memqueue *q);
int  memqueue_detch(struct memqueue *q);
int  memqueue_isdetched(struct memqueue *q);
int  memqueue_is_empty(struct memqueue *q);
int  memqueue_is_full(struct memqueue *q);
int  memqueue_maxlen(struct memqueue *q);
int  memqueue_datalen(struct memqueue *q);
int  memqueue_vvlen(struct memqueue *q);
char* memqueue_padd(struct memqueue *q, char *p, u32 len);

#ifdef _cplusplus
}
#endif


#endif
