#include "memqueue_pool.h"
#include <time.h>
#include "timer.h"
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>


struct memqueue  *sslqueue_push;
struct memqueue  *sslqueue_pop;
struct memqueue_pool pool;
int push_len = 0;
int bad_count = 0;
int bad_pop_count = 0;
int bad_push_count = 0;


#define  test_len (800 *1024U)
#define NANO_TIME (100)


int my_memcpy(char *dst, char *src, size_t len)
{
    while (len--) {
        *dst++ = *src++;
    }

    return 0;
    
}

void log_pkt(unsigned char *buf, int len)
{
     int j;
    
    if (!buf)
        return;
    

        printf("[soap]-len = %d byte, buf addr: 0x%p", len, buf);
        for (j = 0; j < len; j++) {
            if ((j % 16) == 0)
                printf("\n %03x:", j);
            printf(" %02x", buf[j]);
        }
        printf("\n");

}

int sslanalyse(struct memqueue *q,char *data, u32 *len); //mark start and end
 
int sslanalyse(struct memqueue *q,char *data, u32 *len)
{   
    char *tmp = NULL;
    char sz_tmp_data[4] = {0};
    int tmp_len = 0;
    
	if (!data) {
		return -1;
	}
    if (data > q->tail - sizeof(u32)) {
//        printf("sslqueue %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
        tmp = memqueue_padd(q, data, sizeof(u32));
        memcpy(sz_tmp_data, data, q->tail-data);
        memcpy(sz_tmp_data + (q->tail-data), q->head, tmp - q->head);
        memcpy(&tmp_len, sz_tmp_data, sizeof(u32));
        tmp_len = tmp_len;
 //       printf("sslqueue error %s %s %d tmp_len %d \n", __FILE__,__FUNCTION__, __LINE__, tmp_len);
    }
    else {      
	    tmp_len = *(u32*)(data);
    }

    *len = tmp_len + sizeof(u32);
    
	if (*len == sizeof(u32)) return -1;

	return 0;
}



 int sslanalyse2(struct memqueue *q,char *data, u32 *len)
 {
    if (!data) {
		return -1;
	}

	*len = 1500;
	return 0;
 }


 void *pop_thread()
 {
	
    prctl(PR_SET_NAME, (u64)"pop_thread");
    char data_ret[65535]={0};
     u32 len_pop = 0;
     u32 len = 0;
     u32 ret = 0 ;
     static u32 recv_count = 0;
     char *pptr = NULL;

      struct timespec tmsec = {.tv_sec = 0, .tv_nsec = NANO_TIME};
      
     
     sslqueue_pop = memqueue_pool_get_queue(&pool, (void *)1);
     if (!sslqueue_pop) {
         printf("sslqueue NULL %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
          //continue;
          return NULL;
     }

     printf("sslqueue pop_thread %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);


     while(1){
         len_pop = 0;
         len = 0;
         ret = 0 ;
         //sleep(1);
         pptr = memqueue_get_prepopbuff_continuely(sslqueue_pop, &len_pop);
         memqueue_get_prepopbuff_continuely_recorrect(sslqueue_pop);
     
         if(pptr != NULL){
         
            recv_count++;
            if (0 == recv_count % 100000 /*2*/) {
                printf("pop  line = %3d, recv_count %d  len_pop  %d\n",__LINE__, recv_count, len_pop);
            }
            
             //log_pkt(sslqueue_pop->head, 128);
         }
        else {
            bad_pop_count ++;
            //return NULL;
            //usleep(3);
            // nanosleep(&tmsec, NULL);
             //printf("-1  error len_pop= %d line = %d, pop_count = %d\n",len_pop,__LINE__, recv_count);
              
        }
     }
 }


 void *speed_thread()
 {
     static int len = 0;
     static int local_bad_pop_count = 0;
     static int local_bad_push_count = 0;
 
     while (1) {
         len = push_len;
         local_bad_pop_count = bad_pop_count;
         local_bad_push_count = bad_push_count;
         sleep(1);
         printf("send speed is:%d k/s , pop_lock is:%d c/s ,push_lock is:%d c/s \n", 
          (((push_len - len)))/1024, (((bad_pop_count - local_bad_pop_count))), (((bad_push_count - local_bad_push_count))));
     }
 }

 int speed_func(void *arg, int args)
 {
     static int len = 0;
     static int local_bad_pop_count = 0;
     static int local_bad_push_count = 0;

    printf("send speed is:%d k/s , pop_lock is:%d c/s ,push_lock is:%d c/s \n", 
     (((push_len - len))), (((bad_pop_count - local_bad_pop_count))), (((bad_push_count - local_bad_push_count))));
    len = push_len;
    local_bad_pop_count = bad_pop_count;
    local_bad_push_count = bad_push_count;


    //timerTaskStart(0);     /*¹Ø±Õ¶¨Ê±Æ÷*/

    return 0;

 }


void *memtest(void *arg)
{
    char data[128 * 1024] = {5};
    char data_ret[128 * 1024] = {0};
    memcpy(data, data_ret, sizeof(data));
    
    push_len ++;
}

 void *push_thread()
  {

    char data[test_len] = {5};
	char data_ret[test_len] = {0};
	u32 len = 2;
	u32 len_pop = 0;
	u32 push_count = 0;
	char *pushptr = NULL;
      struct timespec tmsec = {.tv_sec = 0, .tv_nsec = NANO_TIME};
#if 1
        sslqueue_push = memqueue_pool_get_queue(&pool, (void *)1);
    if (!sslqueue_push) {
        printf("sslqueue error %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
         //continue;
    }

    
    printf("sslqueue  %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);

    while (1) {
        (len = (++len) % 1600)+128000;
        if (len == 0) {
            len++;
        }
        memset(data, 0xff, test_len);
        memcpy(data, &len, sizeof(len));
    
        pushptr = memqueue_get_prepushbuff_continuely(sslqueue_push, 4+len);
        if (pushptr != NULL) {
            memcpy(pushptr, &len, 4);
            memcpy(pushptr+4, data, len);
            memqueue_get_prepushbuff_continuely_recorrect(sslqueue_push, 0);
            push_len += len;
            push_count++;
            
            if (push_count % 100000 /*2*/ == 0)
                printf("push line = %3d, push_count %d, len_push %d\n",__LINE__, push_count, len +4);
        }
        else {
            memqueue_get_prepushbuff_continuely_recorrect(sslqueue_push, 0);
            bad_push_count ++;
        }
        
    
        #if 0
        while (memqueue_push(sslqueue_push, data, 4+len)) {
            //log_pkt(sslqueue_push->head, 128);
            bad_push_count ++;
            
            //nanosleep(&tmsec, NULL);
           
        };
          
       push_len ++;
       // while (memqueue_push(sslqueue_push, data, 1500)) {
           //printf("push = %d, count %d, len %d\n",__LINE__, push_count, len +4);  
          //  log_pkt(sslqueue_push->head, 128);
        //};
        if (push_count % 100000 == 0)
            printf("push line = %3d, push_count %d, len_push %d\n",__LINE__, push_count, len +4);

        push_count++;

#endif
        //read_thread();

    }
#endif
      
  }



 
 int main(int argc, char *argv[])
 {

	char data[test_len] = {5};
	char data_ret[test_len] = {0};
	u32 len = 2;
	u32 len_pop = 0;
	char *tmp = NULL;
	char *push_buff = NULL;
	char *pop_buff = NULL;
	u32 push_count = 0;
	memset(data, 0xff, test_len);
	printf("sslqueue  %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
	if (memqueue_pool_init(&pool, 1, test_len, 0,  sslanalyse, (void *)1))  {
		printf("sslqueue error %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
		return -1;
	}
	printf("sslqueue  %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);

    tmp = (char *)malloc(6);


        
    if (timerTaskStart(1)) {
        printf("sslqueue  %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
        return -1;
    }

    if (timerAdd(1, speed_func, NULL, 0, 1) < 0) {
        printf("sslqueue  %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
        return -1;
    }


    //pthread_t thread_speed;
    //pthread_create(&thread_speed, NULL, speed_thread, NULL);


    //pthread_join(thread_speed, NULL);



    //pthread_t thread_print;
    //pthread_create(&thread_print, NULL, speed_thread, NULL); 
    //pthread_join(thread_print, NULL);
#if 1
    
	int const pcnt_push = 1;
	int const pcnt_pop = 1;
	int index = 0;
	
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

	pthread_t thread_push[pcnt_push];
	pthread_t thread_pop[pcnt_pop];
	
	for (index = 0; index < pcnt_push;index++) {
		pthread_create(&thread_push[index], &a, push_thread, NULL);
	}
	

    for (index = 0; index < pcnt_pop;index++) {
		pthread_create(&thread_pop[index], &a, pop_thread, NULL);
	}

   #endif
   
    #if 1 
 #if 0
    pthread_t thread_push;
    pthread_create(&thread_push, NULL, push_thread, NULL);
    //pthread_join(thread_push, NULL);
    //pthread_setschedprio(thread_push, 15);

    



    pthread_t thread_pop;
    pthread_create(&thread_pop, NULL, pop_thread, NULL);
    //pthread_setschedprio(thread_pop, 16);
    //pthread_join(thread_pop, NULL);


    pthread_t thread_pop2;
    pthread_create(&thread_pop2, NULL, pop_thread, NULL);
    //pthread_setschedprio(thread_pop, 16);
    //pthread_join(thread_pop, NULL);

    pthread_t thread_pop3;
    pthread_create(&thread_pop3, NULL, pop_thread, NULL);
    //pthread_setschedprio(thread_pop, 16);
    //pthread_join(thread_pop, NULL);




    pthread_t thread_push2;
    pthread_create(&thread_push2, NULL, push_thread, NULL);
    //pthread_join(thread_push, NULL);
    //pthread_setschedprio(thread_push, 15);

    pthread_t thread_pop2;
    pthread_create(&thread_pop2, NULL, pop_thread, NULL);
    //pthread_setschedprio(thread_pop, 16);
    //pthread_join(thread_pop, NULL);



    pthread_t thread_push3;
    pthread_create(&thread_push3, NULL, push_thread, NULL);
    //pthread_join(thread_push, NULL);
    //pthread_setschedprio(thread_push, 15);

    pthread_t thread_pop3;
    pthread_create(&thread_pop3, NULL, pop_thread, NULL);
    //pthread_setschedprio(thread_pop, 16);
    //pthread_join(thread_pop, NULL);



    #endif 
    
#else

    //read_thread();
           sslqueue_push = memqueue_pool_get_queue(&pool, (void *)1);
        if (!sslqueue_push) {
            printf("sslqueue error %s %s %d \n", __FILE__,__FUNCTION__, __LINE__);
             //continue;
             return 0;
        }
    while (1) {
  
        //sleep(1);
           (len = (++len) % 1600)+1;
           if (len == 0) {
               len++;
           }
         
            #if 0

           push_buff = memqueue_get_prepushbuff_continuely(sslqueue_push, 1700);
           if (!push_buff) {
               printf("push error line = %d, push_count %d, len_push %d\n",__LINE__, push_count, len +4);
                continue;
           }

    
           memcpy(push_buff, &len, sizeof(len));
           memcpy(push_buff+4, data, len);
           memqueue_get_prepushbuff_continuely_recorrect(sslqueue_push, 1700 - (4+len));


           while (1) {
                    
               pop_buff = memqueue_get_prepopbuff_continuely(sslqueue_push, &len_pop);
               if (!pop_buff) {
                    //printf("pop_buff error line = %d, push_count %d, len_push %d\n",__LINE__, push_count, len +4);
                    break;
               }    

               if (memqueue_rst_data_end(sslqueue_push)) {
                    printf("pop_buff error line = %d, push_count %d, len_push %d\n",__LINE__, push_count, len +4);
               }
           }

#else
              //memset(data, 0xff, test_len);
           memcpy(data, &len, sizeof(len));
           memcpy(data+sizeof(len), &data_ret, len);
           
           while (memqueue_push(sslqueue_push, data, 4+len)) {
               //log_pkt(sslqueue_push->head, 128);
               bad_count++;
              
           };
           

        while (memqueue_pop(sslqueue_push, data_ret, &len_pop)) {
            //bad_count++;
        };
#endif

        if (push_count % 100000 == 0)
            printf("push line = %d, push_count %d, len_push %d\n",__LINE__, push_count, len +4);

        push_count++;

        push_len += 4+len;
        //printf("push line = %d, push_count %d, len_push %d\n",__LINE__, push_count, len +4);
  
        //read_thread();

    }
#endif

  

while (1){
    //pop_thread(NULL);
    sleep(1000);
};

     memqueue_pool_deinit(&pool);
     printf("push line = %d, count %d, len %d\n",__LINE__, push_count++, 0);


return 0;

 }
 
 
