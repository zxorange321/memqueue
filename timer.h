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
  整理目录

  Revision 1.2  2011/03/28 10:01:41  cgd
  增加snmp 的vlan和虚接口

  Revision 1.1  2011/01/25 03:23:56  cgd
  全面整理代码

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


#define MAX_BUCKET_SIZE       8192      /*Timer 的Bucket桶深保证两个G∈钡亩ㄊ�*/
#define MAX_TIMER_SIZE          1024               /*最大用户(定时器)蔵*/
#define BUCKET_MASK                (MAX_BUCKET_SIZE-1)
/*单个定时器*/
typedef struct timerEvent
{
    struct timerEvent *pNext; 
    struct timerEvent *pre; 
    int timerValue;                /*多长时间sec*/
    int key;                                 /*识眊timer的关键字*/
    int (*callBack)(void*,int); /*超时回调执GG函蔵*/
    void *arg1;                          /*参蔵*/
    int arg2;                               /*参蔵长度*/
    int if_reop;                           /*0表示只执GG一次，1继G�*/
}TIMER_EVENT;

/*单个箂希桶*/
typedef struct timer_head
{
    TIMER_EVENT *bucket_head;
    TIMER_EVENT *bucket_tail;
    int bucket_timerNum;                     /*当前桶覩多少个定时器*/
    
}TIMER_HEAD;

/*任务处理链表*/
struct timer_task
{
    TIMER_EVENT *task_head;
    TIMER_EVENT *task_tail;
    
};

struct bucket_key_manger{
    int if_user;            /*是否在使用*/  
    int bucket;                          /*属于哪一个hash桶*/ 
};

extern TIMER_EVENT * timer_task_get_tail();
extern void timer_task_Process();
extern void timerAddToBucket(int bucket, TIMER_EVENT *pTimer);
extern int timerAdd(int second,int (*callBack)(void*,int), void *user_data,int len,int if_reop);
extern void timerStop(int key);
extern void timerProcess(void);
extern int timerTaskStart(int isStart);
#endif
