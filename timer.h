/*
  $Id: ac_timer.h,v 1.1 2015/03/08 10:28:15 xiaolj Exp $
  $Author: xiaolj $
  $Date: 2015/03/08 10:28:15 $
  $Log: ac_timer.h,v $
  Revision 1.1  2015/03/08 10:28:15  xiaolj
  *** empty log message ***

  Revision 1.1  2014/07/22 02:08:13  xiaolj
  first

  Revision 1.1  2012/03/12 09:26:47  cvsadmin
  first uploaded

  Revision 1.1  2011/06/20 12:34:18  cgd
  ����Ŀ¼

  Revision 1.2  2011/03/28 10:01:41  cgd
  ����snmp ��vlan����ӿ�

  Revision 1.1  2011/01/25 03:23:56  cgd
  ȫ���������

*/
#ifndef __SHIYAN_TIMER__
#define  __SHIYAN_TIMER__

#include <semaphore.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>


#define MAX_BUCKET_SIZE       8192      /*Timer ��BucketͰ�֤����G�ʱ�Ķ�ʱ*/
#define MAX_TIMER_SIZE          1024               /*����û�(��ʱ��)�i*/
#define BUCKET_MASK                (MAX_BUCKET_SIZE-1)
/*������ʱ��*/
typedef struct timerEvent
{
    struct timerEvent *pNext; 
    struct timerEvent *pre; 
    int timerValue;                /*�೤ʱ��sec*/
    int key;                                 /*ʶ�gtimer�Ĺؼ���*/
    int (*callBack)(void*,int); /*��ʱ�ص�ִGG���i*/
    void *arg1;                          /*���i*/
    int arg2;                               /*���i����*/
    int if_reop;                           /*0��ʾִֻGGһ�Σ�1��G�ѭ��*/
}TIMER_EVENT;

/*�����sϣͰ*/
typedef struct timer_head
{
    TIMER_EVENT *bucket_head;
    TIMER_EVENT *bucket_tail;
    int bucket_timerNum;                     /*��ǰͰ�G���ٸ���ʱ��*/
    
}TIMER_HEAD;

/*����������*/
struct timer_task
{
    TIMER_EVENT *task_head;
    TIMER_EVENT *task_tail;
    
};

struct bucket_key_manger{
    int if_user;            /*�Ƿ���ʹ��*/  
    int bucket;                          /*������һ��hashͰ*/ 
};

extern TIMER_EVENT * timer_task_get_tail();
extern void timer_task_Process();
extern void timerAddToBucket(int bucket, TIMER_EVENT *pTimer);
extern int timerAdd(int second,int (*callBack)(void*,int), void *user_data,int len,int if_reop);
extern void timerStop(int key);
extern void timerProcess(void);
extern int timerTaskStart(int isStart);
#endif
