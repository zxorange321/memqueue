#include "memqueue_pool.h"

int  memqueue_pool_init(struct memqueue_pool *pool, u32 count, u32 len, u32 reserved,
    analysefunc  analyse, void *pool_owner)
{	
	struct memqueue *q = NULL;
	int index  = 0;
	if (!pool) {
		return -1;
	}
	
	#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_init(&pool->lock, PTHREAD_PROCESS_PRIVATE);
#else
    pthread_mutex_init(&pool->lock, NULL);
#endif  
	pool->owner = pool_owner;
	INIT_HLIST_HEAD(&pool->head);
	
	for (index = 0; index < count; index++) {
		q = (struct memqueue *)malloc(sizeof(struct memqueue));
		if (!q) {
			return -1;
		}
		
		if (memqueue_init(q, len, reserved, analyse)) {
			free(q);
			return -1;
		}	
		hlist_add_head(&q->list, &pool->head);
	}
	return 0;
}


int  memqueue_pool_deinit(struct memqueue_pool *pool)
{
	struct hlist_node *node;
    struct memqueue * item;            
        #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        pthread_spin_lock(&pool->lock);   
    #else
        pthread_mutex_lock(&pool->lock);    
    #endif      
    if (!hlist_empty(&pool->head)) {   
        hlist_for_each(node, &pool->head) {
            item = hlist_entry(node, struct memqueue, list);    
            memqueue_deinit(item); 
            free(item);
        }   
    }   
        #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_unlock(&pool->lock);   
    #else
    pthread_mutex_unlock(&pool->lock);    
    #endif 
	
	#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_destroy(&pool->lock);
#else
    pthread_mutex_destroy(&pool->lock);
#endif 
	return 0;

}


struct memqueue *memqueue_pool_get_queue(struct memqueue_pool *pool, void *queue_owner)
{
	struct hlist_node *node, *noden;
    struct memqueue *tmp = NULL;  
    int flag = 0;
        #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        pthread_spin_lock(&pool->lock);   
        #else
        pthread_mutex_lock(&pool->lock);    
        #endif   
    hlist_for_each_safe(node, noden, &pool->head) {
		tmp = hlist_entry(node, struct memqueue, list); 
		flag = 0;
        #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        //printf("sslqueue before %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
        pthread_spin_lock(&tmp->lock);   
        //printf("sslqueue after %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
        #else
        pthread_mutex_lock(&tmp->lock);    
        #endif   
		
        if (queue_owner && memqueue_owner(tmp)) { 
			flag = (memqueue_owner(tmp) == queue_owner) ? 1 : 0;
            if (!flag) { 
                printf("sslqueue tmp %s %s %d queue_owner %p  tmp %p flag %d threadid %lx\n", __FILE__,__FUNCTION__, __LINE__, queue_owner, memqueue_owner(tmp), flag, pthread_self()); 
            }
 		} else if (queue_owner && !memqueue_owner(tmp)) {
			flag = (memqueue_bind(tmp, queue_owner) < 0) ? 0 : 1;		
            if (!flag) {       
                printf("sslqueue tmp %s %s %d queue_owner %p  tmp %p flag %d threadid %lx\n", __FILE__,__FUNCTION__, __LINE__, queue_owner, memqueue_owner(tmp), flag, pthread_self()); 
            }
		} else {
            flag = 0;
            if (!flag) { 
                printf("sslqueue tmp %s %s %d queue_owner %p  tmp %p flag %d threadid %lx\n", __FILE__,__FUNCTION__, __LINE__, queue_owner, memqueue_owner(tmp), flag, pthread_self()); 
            }

        }

#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
        //printf("sslqueue before %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
        pthread_spin_unlock(&tmp->lock); 
        //printf("sslqueue after %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
#else
        pthread_mutex_unlock(&tmp->lock);    
#endif 

		if (flag) {
#ifdef MEMQUEUE_SPIN_LOCK_DEFINE
            pthread_spin_unlock(&pool->lock);   
#else
            pthread_mutex_unlock(&pool->lock);    
#endif 
            return tmp;
		}
		
    }

    #ifdef MEMQUEUE_SPIN_LOCK_DEFINE
    pthread_spin_unlock(&pool->lock);   
    #else
    pthread_mutex_unlock(&pool->lock);    
    #endif 
    if (!flag) { 
        printf("sslqueue tmp %s %s %d queue_owner %p  tmp %p flag %d threadid %lx\n", __FILE__,__FUNCTION__, __LINE__, queue_owner, memqueue_owner(tmp), flag, pthread_self()); 
    }

	return NULL;
}
