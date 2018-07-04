/*
  $Id: ac_timer.c,v 1.1 2015/03/08 10:28:15 xiaolj Exp $
  $Author: xiaolj $
  $Date: 2015/03/08 10:28:15 $
  $Log: ac_timer.c,v $
  Revision 1.1  2015/03/08 10:28:15  xiaolj
  *** empty log message ***

  Revision 1.4  2014/11/09 13:36:37  licj
  *** empty log message ***

  Revision 1.3  2014/09/16 05:28:46  xiaolj
  alter timer_task_Process

  Revision 1.2  2014/09/04 02:29:25  xiaolj
  nothing

  Revision 1.1  2014/07/22 02:08:13  xiaolj
  first

  Revision 1.1  2012/03/12 09:26:47  cvsadmin
  first uploaded

  Revision 1.7  2011/08/24 01:34:08  cgd
  解决bug456

  Revision 1.6  2011/08/15 02:01:25  cgd
  nothing

  Revision 1.5  2011/07/05 09:07:55  cgd
  退出修改

  Revision 1.4  2011/04/15 09:31:59  cgd
  trap开关||loadblance bug

  Revision 1.3  2011/04/07 09:17:57  cgd
  修改释放hash桶定时器

  Revision 1.2  2011/03/28 09:57:37  cgd
  修改以前的定时管理机制

  Revision 1.1  2011/01/25 03:22:58  cgd
  整理代码

*/
#include "timer.h"
#include <time.h>
#include <signal.h>
/*Private Members*/
TIMER_HEAD *gTimerBucket = NULL;    /*Timer Bucket的首地址*/
struct timer_task gTimertask ;  /*任务运行链表首地址*/
int gCurBucket = 0;             /*当前指向的Timer Bucket*/
int gTimerIsRunning = 0;        /*Timer处理任务是否在运行*/
pthread_mutex_t gTimerBucket_mutex; /*Timer bucket 锁*/
sem_t gTimerSemID;                   /*Timer任务同步信号量*/
sem_t gTimerBucketSemID;          /*hash桶停止循环信号*/
sem_t gTimertaskSemID;                   /*任务停止通知信号*/
pthread_t gTimerBucket_thread_id;
pthread_t gTimertask_thread_id;
struct bucket_key_manger *key_manger;/*key值管理*/

extern int ac_if_run;
extern void sig_exit(int signo);
int mysleep(int second,long nsec){
    if(second==0 && nsec ==0)
        return -1;
    sigset_t sigset;
    if(sigemptyset(&sigset) == -1){
        //ACCW_LOG("sigset init filed");
        return -1;
    }
    if(sigaddset(&sigset,SIGALRM) == -1){
        //ACCW_LOG("SIGALRM added sigset filed");
        return -1;
    }
    /*
    if(sigaddset(&sigset,SIGTERM) == -1){
        return -1;
    }
    */
    struct timespec timeout;
    timeout.tv_sec = second;
    timeout.tv_nsec = nsec;
    return pselect(0, NULL, NULL, NULL, &timeout,&sigset);
}
TIMER_EVENT * timer_task_get_tail(){

    TIMER_EVENT *temp = NULL;
    if(gTimertask.task_tail == NULL){
        temp = NULL;
    }else{
        if(gTimertask.task_tail == gTimertask.task_head){
            temp = gTimertask.task_tail;
            gTimertask.task_tail = gTimertask.task_head = NULL;
        }else{
            temp = gTimertask.task_tail;
            gTimertask.task_tail = temp->pre;
            gTimertask.task_tail->pNext = NULL;
            
        }
        temp->pre =NULL;
        temp->pNext = NULL;
    }

    return temp;
}
void timer_task_Process(){
    TIMER_EVENT *temp = NULL;
    while(1){
        sem_wait(&gTimerSemID);
        if(gTimerIsRunning == 0)
            break;
        pthread_mutex_lock(&gTimerBucket_mutex);
        if((temp = timer_task_get_tail()) == NULL){
            pthread_mutex_unlock(&gTimerBucket_mutex);
            continue;
        }
        else{   
            //执行回调函数
            pthread_mutex_unlock(&gTimerBucket_mutex);
            (*temp->callBack)(temp->arg1,temp->arg2);
            pthread_mutex_lock(&gTimerBucket_mutex);

        }
        
        if(temp->if_reop){//继续循环
            int selectBucket = (gCurBucket + temp->timerValue) & (BUCKET_MASK);
            key_manger[temp->key].bucket = selectBucket;
            timerAddToBucket(selectBucket,temp);
        }else{
            if(temp->arg2 > 0)
                free(temp->arg1);
            free(temp);
        }
        temp = NULL;
        pthread_mutex_unlock(&gTimerBucket_mutex);
    }
    /*清除任务中所有的定时器*/
    pthread_mutex_lock(&gTimerBucket_mutex);
    temp = gTimertask.task_head;
    TIMER_EVENT *p = NULL;
    while(temp){
        p = temp->pNext;
        if(temp->arg2 > 0)
            free(temp->arg1);
        free(temp);
        temp = p;
    }
    gTimertask.task_head = NULL;
    gTimertask.task_tail = NULL;
    pthread_mutex_unlock(&gTimerBucket_mutex);
    sem_post(&gTimertaskSemID);  //线程结束通知
    return;
}
void timerAddToBucket(int bucket, TIMER_EVENT *pTimer)
{
    if (gTimerBucket == NULL || gTimerIsRunning == 0) {
        return;
    }   
    TIMER_EVENT * pHeadTimer = gTimerBucket[bucket].bucket_head;
    if(pHeadTimer == NULL){
        gTimerBucket[bucket].bucket_head = pTimer;
        gTimerBucket[bucket].bucket_tail = pTimer;
    }else{
        pTimer->pNext = pHeadTimer;
        pHeadTimer->pre = pTimer;
        gTimerBucket[bucket].bucket_head = pTimer;
    }
    
    gTimerBucket[bucket].bucket_timerNum ++;

}

/********************************************************************************
 * 功能     : 增加一个定时器，到期时执行callBack函数
 *    
 * 参数     : 
 * [IN] 
 *   second - 定时时间
 *   callBack - 定时事件处理函数
 * [OUT]
 *   无
 * 
 * 返回值   : 
 *   0  - 错误
 *   非0 - timer的句柄，用户关闭时使用
*******************************************************************************/  
int timerAdd(int second,int (*callBack)(void*,int), void *user_data,int len,int if_reop)
{
    int selectBucket = 0;
    //int bucketMask = MAX_BUCKET_SIZE - 1;
    TIMER_EVENT *pTimer;
    int key = 0;
    if ((pTimer = (struct timerEvent *)malloc(sizeof(struct timerEvent))) == NULL)
    {
        return -1;      
    }
    memset(pTimer, 0, sizeof(TIMER_EVENT));
    pTimer->callBack = callBack;
    pTimer->timerValue = second;
    pTimer->if_reop = if_reop;
    if(user_data != NULL) {

        if(len > 0){
            pTimer->arg1 = malloc(len);
            if(pTimer->arg1 == NULL){
                free(pTimer);
                return -1;
            }
            memcpy(pTimer->arg1, user_data, len);
            pTimer->arg2= len;
        }else
            pTimer->arg1 = user_data;
    }
    
    /*通过"与",实现了select_bucket超过bucket_mask 时,自动回绕. */
    selectBucket = (gCurBucket + second) & (BUCKET_MASK);
    if (gTimerBucket == NULL || gTimerIsRunning == 0) {
        if (pTimer->arg2) {
            free(pTimer->arg1);
        }
        return -1;
    }
    pthread_mutex_lock(&gTimerBucket_mutex);
    while(key < MAX_TIMER_SIZE){
        if(key_manger[key].if_user == 0)
            break;
        key ++;
    }
    key_manger[key].if_user = 1;
    key_manger[key].bucket = selectBucket;
    pTimer->key = key;
    timerAddToBucket(selectBucket,pTimer);
    
    pthread_mutex_unlock(&gTimerBucket_mutex);

    return key;
}

void timerStop(int key)
{
    TIMER_EVENT *pTimer = NULL;
    int selectBucket = 0;
    
    pthread_mutex_lock(&gTimerBucket_mutex);
    /*查找该timer是否存在*/
    if(key_manger[key].if_user == 0){
        pthread_mutex_unlock(&gTimerBucket_mutex);
        return;
    }
    /*先在hash桶里找*/
    selectBucket = key_manger[key].bucket;
    key_manger[key].bucket = 0;
    key_manger[key].if_user = 0;
    pTimer = gTimerBucket[selectBucket].bucket_head;
    while(pTimer)
    {
        if(pTimer->key == key)
        {
            if(pTimer == gTimerBucket[selectBucket].bucket_head){//=head
                if(pTimer == gTimerBucket[selectBucket].bucket_tail){
                    gTimerBucket[selectBucket].bucket_head = NULL;
                    gTimerBucket[selectBucket].bucket_tail = NULL;
                }else{
                    gTimerBucket[selectBucket].bucket_head = pTimer->pNext;
                    pTimer->pNext->pre = NULL;
                }
            }else if(pTimer == gTimerBucket[selectBucket].bucket_tail){
                
                gTimerBucket[selectBucket].bucket_tail = pTimer->pre;
                pTimer->pre->pNext = NULL;
            }else{       //zhongjian
                pTimer->pNext->pre = pTimer->pre;
                pTimer->pre->pNext = pTimer->pNext;
                
            }
            break;
        }
        pTimer = pTimer->pNext;
    }
    if(pTimer == NULL){// 继续在任务链里面找
        pTimer = gTimertask.task_head;
        while(pTimer){
            if(pTimer->key == key){
                if(pTimer == gTimertask.task_head){
                    if(pTimer == gTimertask.task_tail){
                        gTimertask.task_tail = NULL;
                        gTimertask.task_head = NULL;
                    }else{
                        gTimertask.task_head = pTimer->pNext;
                        pTimer->pNext->pre = NULL;
                    }
                }else if(pTimer == gTimertask.task_tail){
                    gTimertask.task_tail = pTimer->pre;
                    pTimer->pre->pNext = NULL;
                }else{
                    pTimer->pNext->pre = pTimer->pre;
                    pTimer->pre->pNext = pTimer->pNext;
                }
                break;
            }
            
            pTimer = pTimer->pNext;
        }
    }
    if(pTimer == NULL){
        pthread_mutex_unlock(&gTimerBucket_mutex);
        return;
    }else{
        if(pTimer->arg2 > 0)
            free(pTimer->arg1);
        free(pTimer);
        pTimer = NULL;
    }
    pthread_mutex_unlock(&gTimerBucket_mutex);
    return;
}

/********************************************************************************
 * 功能     : 定时器检查函数，将定时到的事件JIA 如到任务队列
 *    
 * 参数     : 
 * [IN] 
 *   无
 *   
 * [OUT]
 *   无
 * 
 * 返回值   : 
 *   无
 *   
*******************************************************************************/  
void timerProcess(void)
{
    TIMER_EVENT *phead = NULL;
    TIMER_EVENT *ptail = NULL;
    //TIMER_EVENT *temp = NULL;
    int timer_num = 0;
    //int selectBucket = 0;

    while(gTimerIsRunning)
    {
        
            
        sleep(1);/*秒级时间精度*/
        //mysleep(1,0);//850000000
        //if(gTimerIsRunning == 0 ||ac_if_run == 0)
        if(gTimerIsRunning == 0 )
            break;
        pthread_mutex_lock(&gTimerBucket_mutex);
        phead = gTimerBucket[gCurBucket].bucket_head;
        ptail = gTimerBucket[gCurBucket].bucket_tail;
        /*将所有超时的Timer对应的事件加入到事件队列中*/
        if(phead && ptail)
        {   /*下链操作*/
            gTimerBucket[gCurBucket].bucket_head = NULL;
            gTimerBucket[gCurBucket].bucket_tail = NULL;
            timer_num = gTimerBucket[gCurBucket].bucket_timerNum;
            gTimerBucket[gCurBucket].bucket_timerNum = 0;
            /*上链操作*/
            if(gTimertask.task_tail == NULL){/*KONG lian*/
                gTimertask.task_head = phead;
                gTimertask.task_tail = ptail;

            }else{
                ptail->pNext = gTimertask.task_head;
                gTimertask.task_head->pre = ptail;
                gTimertask.task_head = phead;
            }
        }
        while(timer_num){
            sem_post(&gTimerSemID);
            timer_num --;
        }
        pthread_mutex_unlock(&gTimerBucket_mutex);
        
        /*通过"与",实现了g_garpCurBucket超过bucket_mask 时,自动回绕*/
        gCurBucket = (gCurBucket + 1) & BUCKET_MASK; 

    }
    /*线程退出*/
    
    pthread_mutex_lock(&gTimerBucket_mutex);
    int i;
    for(i = 0; i<MAX_BUCKET_SIZE; i++){
        phead = gTimerBucket[i].bucket_head;
        while(phead){
            ptail = phead->pNext;
            if(phead->arg2 > 0)
                free(phead->arg1);
            free(phead);
            phead = ptail;
        }
        gTimerBucket[i].bucket_head = NULL;
        gTimerBucket[i].bucket_tail= NULL;
    }   
    pthread_mutex_unlock(&gTimerBucket_mutex);
    sem_post(&gTimerBucketSemID);  //线程结束通知
    //if (ac_if_run == 0)
        //sig_exit(0);
    return ;
}

/********************************************************************************
 * 功能     : 停止定功能开启关闭函数
 *    
 * 参数     : 
 * [IN] 
 *   isStart - 1，开启；0，关闭
 *
 * [OUT]
 *   无
 * 
 * 返回值   : 
 *   OK       -   处理成功
 *   ERROR    -   处理失败
*******************************************************************************/  
int timerTaskStart(int isStart)
{
    if (isStart)
    {
        if (gTimerIsRunning)
        {
            return 0;
        }
                gTimerBucket = NULL;
                gCurBucket = 0; 
                memset(&gTimertask,0,sizeof(struct timer_task));
                key_manger = NULL;
        gTimerIsRunning = 1;

        if ((gTimerBucket = (struct timer_head *)malloc(MAX_BUCKET_SIZE * sizeof(TIMER_HEAD))) == NULL)
        {
            goto Timer_out;     
        }
        memset(gTimerBucket, 0, MAX_BUCKET_SIZE * sizeof(TIMER_HEAD));

        if((key_manger = (struct bucket_key_manger *)malloc(MAX_TIMER_SIZE*sizeof(struct bucket_key_manger))) == NULL){

            goto Timer_out;
        }
        pthread_mutex_init(&gTimerBucket_mutex, NULL); 


        if(sem_init(&gTimerSemID, 0, 0)!=0) //同步信号量初始化
        {
            goto Timer_out;
        }
        if(sem_init(&gTimerBucketSemID, 0, 0)!=0) //信号量初始化
        {
            goto Timer_out;
        }
        if(sem_init(&gTimertaskSemID, 0, 0)!=0) //信号量初始化
        {
            goto Timer_out;
        }
        gCurBucket = 0;

        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);
        /*创建一个定时线程*/
        if (pthread_create(&gTimerBucket_thread_id,&thread_attr, (void*)timerProcess, NULL) != 0)
        {
            goto Timer_out;
        }
        if (pthread_create(&gTimertask_thread_id,&thread_attr, (void*)timer_task_Process, NULL) != 0)
        {
            goto Timer_out;
        }
        
    }
    else
    {
        if (!gTimerIsRunning)
        {
            return 0;
        }
        Timer_out:
        gTimerIsRunning = 0;

        /*等待子线程结束*/
        if(gTimerBucket_thread_id)
            sem_wait(&gTimerBucketSemID);
        sem_post(&gTimerSemID);//方便运行任务的进程退出
        if(gTimertask_thread_id)
            sem_wait(&gTimertaskSemID);
        /*释放资源*/
        if(gTimerBucket)
            free(gTimerBucket);
        gTimerBucket = NULL;
        if(key_manger)
            free(key_manger);
        key_manger = NULL;
        pthread_mutex_destroy(&gTimerBucket_mutex);
        sem_destroy(&gTimerSemID);
        sem_destroy(&gTimerBucketSemID);
        sem_destroy(&gTimertaskSemID);

        return -1;
    }
    
    return 0;
}


