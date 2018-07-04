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
  ���bug456

  Revision 1.6  2011/08/15 02:01:25  cgd
  nothing

  Revision 1.5  2011/07/05 09:07:55  cgd
  �˳��޸�

  Revision 1.4  2011/04/15 09:31:59  cgd
  trap����||loadblance bug

  Revision 1.3  2011/04/07 09:17:57  cgd
  �޸��ͷ�hashͰ��ʱ��

  Revision 1.2  2011/03/28 09:57:37  cgd
  �޸���ǰ�Ķ�ʱ�������

  Revision 1.1  2011/01/25 03:22:58  cgd
  �������

*/
#include "timer.h"
#include <time.h>
#include <signal.h>
/*Private Members*/
TIMER_HEAD *gTimerBucket = NULL;    /*Timer Bucket���׵�ַ*/
struct timer_task gTimertask ;  /*�������������׵�ַ*/
int gCurBucket = 0;             /*��ǰָ���Timer Bucket*/
int gTimerIsRunning = 0;        /*Timer���������Ƿ�������*/
pthread_mutex_t gTimerBucket_mutex; /*Timer bucket ��*/
sem_t gTimerSemID;                   /*Timer����ͬ���ź���*/
sem_t gTimerBucketSemID;          /*hashͰֹͣѭ���ź�*/
sem_t gTimertaskSemID;                   /*����ֹ֪ͣͨ�ź�*/
pthread_t gTimerBucket_thread_id;
pthread_t gTimertask_thread_id;
struct bucket_key_manger *key_manger;/*keyֵ����*/

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
            //ִ�лص�����
            pthread_mutex_unlock(&gTimerBucket_mutex);
            (*temp->callBack)(temp->arg1,temp->arg2);
            pthread_mutex_lock(&gTimerBucket_mutex);

        }
        
        if(temp->if_reop){//����ѭ��
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
    /*������������еĶ�ʱ��*/
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
    sem_post(&gTimertaskSemID);  //�߳̽���֪ͨ
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
 * ����     : ����һ����ʱ��������ʱִ��callBack����
 *    
 * ����     : 
 * [IN] 
 *   second - ��ʱʱ��
 *   callBack - ��ʱ�¼�������
 * [OUT]
 *   ��
 * 
 * ����ֵ   : 
 *   0  - ����
 *   ��0 - timer�ľ�����û��ر�ʱʹ��
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
    
    /*ͨ��"��",ʵ����select_bucket����bucket_mask ʱ,�Զ�����. */
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
    /*���Ҹ�timer�Ƿ����*/
    if(key_manger[key].if_user == 0){
        pthread_mutex_unlock(&gTimerBucket_mutex);
        return;
    }
    /*����hashͰ����*/
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
    if(pTimer == NULL){// ������������������
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
 * ����     : ��ʱ����麯��������ʱ�����¼�JIA �絽�������
 *    
 * ����     : 
 * [IN] 
 *   ��
 *   
 * [OUT]
 *   ��
 * 
 * ����ֵ   : 
 *   ��
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
        
            
        sleep(1);/*�뼶ʱ�侫��*/
        //mysleep(1,0);//850000000
        //if(gTimerIsRunning == 0 ||ac_if_run == 0)
        if(gTimerIsRunning == 0 )
            break;
        pthread_mutex_lock(&gTimerBucket_mutex);
        phead = gTimerBucket[gCurBucket].bucket_head;
        ptail = gTimerBucket[gCurBucket].bucket_tail;
        /*�����г�ʱ��Timer��Ӧ���¼����뵽�¼�������*/
        if(phead && ptail)
        {   /*��������*/
            gTimerBucket[gCurBucket].bucket_head = NULL;
            gTimerBucket[gCurBucket].bucket_tail = NULL;
            timer_num = gTimerBucket[gCurBucket].bucket_timerNum;
            gTimerBucket[gCurBucket].bucket_timerNum = 0;
            /*��������*/
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
        
        /*ͨ��"��",ʵ����g_garpCurBucket����bucket_mask ʱ,�Զ�����*/
        gCurBucket = (gCurBucket + 1) & BUCKET_MASK; 

    }
    /*�߳��˳�*/
    
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
    sem_post(&gTimerBucketSemID);  //�߳̽���֪ͨ
    //if (ac_if_run == 0)
        //sig_exit(0);
    return ;
}

/********************************************************************************
 * ����     : ֹͣ�����ܿ����رպ���
 *    
 * ����     : 
 * [IN] 
 *   isStart - 1��������0���ر�
 *
 * [OUT]
 *   ��
 * 
 * ����ֵ   : 
 *   OK       -   ����ɹ�
 *   ERROR    -   ����ʧ��
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


        if(sem_init(&gTimerSemID, 0, 0)!=0) //ͬ���ź�����ʼ��
        {
            goto Timer_out;
        }
        if(sem_init(&gTimerBucketSemID, 0, 0)!=0) //�ź�����ʼ��
        {
            goto Timer_out;
        }
        if(sem_init(&gTimertaskSemID, 0, 0)!=0) //�ź�����ʼ��
        {
            goto Timer_out;
        }
        gCurBucket = 0;

        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);
        /*����һ����ʱ�߳�*/
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

        /*�ȴ����߳̽���*/
        if(gTimerBucket_thread_id)
            sem_wait(&gTimerBucketSemID);
        sem_post(&gTimerSemID);//������������Ľ����˳�
        if(gTimertask_thread_id)
            sem_wait(&gTimertaskSemID);
        /*�ͷ���Դ*/
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


