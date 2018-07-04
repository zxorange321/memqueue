#include "memqueue.h"

int  memqueue_init(struct memqueue *q, u32 len, u32 reserved,  analysefunc  analyse)
{
	if (!q) {
		return -1;
	}
	q->analyse = analyse;
	q->len = len;
	q->head = q->data = q->tail = q->end = NULL;
	q->head = (char *)malloc(len);
	memset(q->head, 0, len);
	if (!q->head) {
		return -1;
	}
	q->end = q->data = q->head;
	q->tail = q->head + q->len;	//tail is valid mem
	q->owner = NULL;
	q->reserved = reserved;
    memqueue_reserved(q, q->reserved);

#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_init(&q->lock, PTHREAD_PROCESS_PRIVATE);
#else
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond_push, NULL);
    pthread_cond_init(&q->cond_pop, NULL);
    q->wtime.tv_nsec = 3000;
    q->wtime.tv_sec = 0;
#endif   
	
	INIT_HLIST_NODE(&q->list);
	
	return 0;
}

int  memqueue_deinit(struct memqueue *q)
{
	if (!q) {
		return -1;
	}
	if (q->head) {
		free(q->head);
	}
	
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_destroy(&q->lock);
#else
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond_push);
    pthread_cond_destroy(&q->cond_pop);
#endif 

	return 0;
}



int  memqueue_rst_analyse(struct memqueue *q, analysefunc  analyse)
{
	if (!q) {
		return -1;
	}
	q->analyse = analyse;
	
	return 0;

}

int  memqueue_bind(struct memqueue *q, void *queue_owner)
{
	if (!q ||  !memqueue_isdetched(q)) {
		return -1;
	}

	q->owner = queue_owner;
	
	return 0;
}

void  *memqueue_owner(struct memqueue *q)
{
	if (!q) {
		return NULL;
	}
	
	return q->owner;
}

int  memqueue_detch(struct memqueue *q)
{
	if (!q || !memqueue_is_empty(q)) {
		return -1;
	}
	q->owner = NULL;
	return 0;
}

int  memqueue_isdetched(struct memqueue *q)
{
	if (!q) {
		return -1;
	}
	return q->owner == NULL;
}

int  memqueue_is_empty(struct memqueue *q)
{
	if (!q || !q->analyse) {
		return -1;
	}
	return (q->data == q->end);
}

int  memqueue_is_full(struct memqueue *q)
{
	if (!q || !q->analyse) {
		return -1;
	}
	return (memqueue_padd(q, q->end, 1) == q->data) ;
}

int  memqueue_push(struct memqueue *q, char *mem, u32 len)
{
	char *tmp = NULL;
	if (!q || !q->analyse || !mem) {
		return -1;
	}

#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_lock(&q->lock);   
#else
    pthread_mutex_lock(&q->lock);    
#endif  

    while (1) {
    	if (memqueue_vvlen(q) < len) {
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        pthread_spin_unlock(&q->lock); 
        return -1;
#else
    	pthread_cond_wait(&q->cond_push, &q->lock);
#endif 
    	} else {
            break;
    	}   
	}    

	tmp = memqueue_padd(q, q->end, len);
	if (tmp > q->data || (tmp < q->data && q->end < q->data)) { // 完整的
		memcpy(q->end, mem, len);
	}
    else {  //需要拼接==又返回头部
        if (q->tail - q->end > len) {
            printf("error fragment %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
                #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
            pthread_spin_unlock(&q->lock);   
            #else
            pthread_mutex_unlock(&q->lock);    
            #endif 
            return -1;
            }

        if (len - (q->tail - q->end) > len) {
            printf("error fragment %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
                #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
            pthread_spin_unlock(&q->lock);   
            #else
            pthread_mutex_unlock(&q->lock);    
            #endif 
            return -1;
        }
        
    	memcpy(q->end, mem, q->tail - q->end);
    	memcpy(q->head, mem + (q->tail - q->end), len - (q->tail - q->end));
	}

	
	q->end = tmp;
	
    #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_unlock(&q->lock);   
    #else
    pthread_cond_signal(&q->cond_pop);
    pthread_mutex_unlock(&q->lock);
    #endif 
	
	return 0;
}

int  memqueue_pop(struct memqueue *q, char *mem, u32 *len)
{
	char *tmp = NULL;
	u32 alylen;
	if (!q || !q->analyse || !mem) {
		return -1;
	}
	
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_lock(&q->lock);   
#else
    pthread_mutex_lock(&q->lock);
#endif
    while (1) {
        if (memqueue_is_empty(q)) {
             goto wait;
        }
    
    	if (q->analyse(q, q->data, &alylen) < 0) {
    		goto wait;
    	}
    	if (memqueue_datalen(q) < alylen) {
    		goto wait;
    	}
    	else {
            break;
    	}
 wait:
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        pthread_spin_unlock(&q->lock);
        return -1;
#else       
        pthread_cond_timedwait(&q->cond_pop, &q->lock, &q->wtime);
        //pthread_cond_wait(&q->cond_pop, &q->lock);
#endif 
	}
	
	tmp = memqueue_padd(q, q->data, alylen);
	
	if (tmp >= q->data) { // 完整的
		memcpy(mem, q->data, alylen);
		memset(q->data, 0 ,alylen);
	}
	else {  //需要拼接==又返回头部
		memcpy(mem, q->data, q->tail - q->data);
		memcpy(mem + (q->tail - q->data), q->head, tmp - q->head);
		
		memset(q->data, 0, q->tail - q->data);
		memset(q->head, 0, alylen - (q->tail - q->data));
	}
	q->data = tmp;	
	*len = alylen;

	#if 0
	if (memqueue_is_empty(q)) {
		memqueue_detch(q);
	}
#endif
    #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_unlock(&q->lock);   
    #else
    pthread_cond_signal(&q->cond_push);
    pthread_mutex_unlock(&q->lock); 
    #endif 
	
	return 0;
}

int  memqueue_maxlen(struct memqueue *q)
{
	if (!q || !q->analyse) {
		return -1;
	}
	return q->len;
}

int  memqueue_datalen(struct memqueue *q)
{
	if (!q || !q->analyse) {
		return -1;
	}

	return ((q->end + q->len) - q->data) % q->len;

}

int  memqueue_vvlen(struct memqueue *q) 
{
	if (!q || !q->analyse) {
		return -1;
	}
	
	return ((q->data + q->len) - q->end - 1) % q->len;
}

char* memqueue_padd(struct memqueue *q, char *p, u32 len)
{
	char *tmp = NULL;
	if (!q || !q->analyse || !p) {
		return NULL;
	}
	tmp = p + len;

	return (tmp >= q->tail) ? (q->head + (tmp - q->tail)) : tmp;
}



char *memqueue_get_prepushbuff_continuely(struct memqueue *q, u32 len)
{
	char *tmp = NULL;
	char *tmp2 = NULL;
	float datalen = -1;
	float vvlen = -1;
    int data_len = 0;
	if (!q || !q->analyse) {
		return NULL;
	}

#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_lock(&q->lock);   
#else
    pthread_mutex_lock(&q->lock);    
#endif  

    while (1) {
        if (memqueue_vvlen(q) < len) {
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        return NULL;
#else
        pthread_cond_wait(&q->cond_push, &q->lock);
#endif 
        } else {
            break;
        }   
    }    

    tmp = q->end + len;

    if (tmp < q->tail) { // 完整的
        tmp2 = q->end;
        q->end = tmp;   
        return tmp2;
    }
    else {  //迁徙
        datalen = memqueue_datalen(q);
	    vvlen = memqueue_vvlen(q);
        if (datalen/vvlen < 0.2) {
            memcpy(q->head + q->reserved, q->data, q->end - q->data);
            data_len = q->end - q->data;
            q->data = q->head + q->reserved;       
            q->end = q->data + data_len;
            
            tmp = q->end + len;

            if (tmp < q->tail) { // 完整的
                tmp2 = q->end;
                q->end = tmp;   
                return tmp2;
            }
        }
    }

  return NULL;       
}

int memqueue_get_prepushbuff_continuely_recorrect(struct memqueue *q, u32 len)
{
	if (!q || !q->analyse) {
		return -1;
	}

    q->end -= len;
    
    #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_unlock(&q->lock);   
    #else
    pthread_cond_signal(&q->cond_pop);
    pthread_mutex_unlock(&q->lock); 
    #endif 

    return 0;
}



char *memqueue_get_prepopbuff_continuely(struct memqueue *q, u32 *len) 
{
    char *tmp = NULL;
    char *tmp2 = NULL;
	u32 alylen;
	if (!q || !q->analyse) {
		return NULL;
	}
    
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        pthread_spin_lock(&q->lock);   
#else
        pthread_mutex_lock(&q->lock);
#endif
    while (1) {
        if (memqueue_is_empty(q)) {
            goto wait;
        }
    
    	if (q->analyse(q, q->data, &alylen) < 0) {
    		goto wait;
    	}
    	if (memqueue_datalen(q) < alylen) {
    		goto wait;
    	}
    	else {
            break;
    	}
 wait:
 
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        return NULL;
#else       
       pthread_cond_timedwait(&q->cond_pop, &q->lock, &q->wtime);
       //pthread_cond_wait(&q->cond_pop, &q->lock);
#endif 
	}

	tmp = memqueue_padd(q, q->data, alylen);
	
	if (tmp >= q->data) { // 完整的
        tmp2 = q->data;
        q->data = tmp;
       *len =  alylen;
       return tmp2;
	} 
	else { } //需要拼接==又返回头部	

	return NULL;

}


int  memqueue_get_prepopbuff_continuely_recorrect(struct memqueue *q)
{
    if (!q || !q->analyse) {
        return -1;
    }
    
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_unlock(&q->lock);   
#else
    pthread_cond_signal(&q->cond_push);
    pthread_mutex_unlock(&q->lock); 
#endif 

    return 0;



}



int  memqueue_rst_data_end(struct memqueue *q)
{  
	if (!q || !q->analyse) {
		return -1;
	}

    if (memqueue_is_empty(q)) {
        q->data = q->end = q->head + q->reserved;
        return 0;
    }

    return -1;
}

int  memqueue_reserved(struct memqueue *q, u32 len)
{
	if (!q || !q->analyse) {
		return -1;
	}
	
    q->reserved = len;
    q->data = q->end =  memqueue_padd(q, q->data, q->reserved);
	
    return 0;
}


