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
  ÕûÀíÄ¿Â¼

  Revision 1.2  2011/03/28 10:01:41  cgd
  Ôö¼Ósnmp µÄvlanºÍÐé½Ó¿Ú

  Revision 1.1  2011/01/25 03:23:56  cgd
  È«ÃæÕûÀí´úÂë

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


#define MAX_BUCKET_SIZE       8192      /*Timer µÄBucketÍ°Éî±£Ö¤Á½¸öG¡Ê±µÄ¶¨Ê±*/
#define MAX_TIMER_SIZE          1024               /*×î´óÓÃ»§(¶¨Ê±Æ÷)Êi*/
#define BUCKET_MASK                (MAX_BUCKET_SIZE-1)
/*µ¥¸ö¶¨Ê±Æ÷*/
typedef struct timerEvent
{
    struct timerEvent *pNext; 
    struct timerEvent *pre; 
    int timerValue;                /*¶à³¤Ê±¼äsec*/
    int key;                                 /*Ê¶±gtimerµÄ¹Ø¼ü×Ö*/
    int (*callBack)(void*,int); /*³¬Ê±»Øµ÷Ö´GGº¯Êi*/
    void *arg1;                          /*²ÎÊi*/
    int arg2;                               /*²ÎÊi³¤¶È*/
    int if_reop;                           /*0±íÊ¾Ö»Ö´GGÒ»´Î£¬1¼ÌGøÑ­»·*/
}TIMER_EVENT;

/*µ¥¸ö¹sÏ£Í°*/
typedef struct timer_head
{
    TIMER_EVENT *bucket_head;
    TIMER_EVENT *bucket_tail;
    int bucket_timerNum;                     /*µ±Ç°Í°ÓG¶àÉÙ¸ö¶¨Ê±Æ÷*/
    
}TIMER_HEAD;

/*ÈÎÎñ´¦ÀíÁ´±í*/
struct timer_task
{
    TIMER_EVENT *task_head;
    TIMER_EVENT *task_tail;
    
};

struct bucket_key_manger{
    int if_user;            /*ÊÇ·ñÔÚÊ¹ÓÃ*/  
    int bucket;                          /*ÊôÓÚÄÄÒ»¸öhashÍ°*/ 
};

extern TIMER_EVENT * timer_task_get_tail();
extern void timer_task_Process();
extern void timerAddToBucket(int bucket, TIMER_EVENT *pTimer);
extern int timerAdd(int second,int (*callBack)(void*,int), void *user_data,int len,int if_reop);
extern void timerStop(int key);
extern void timerProcess(void);
extern int timerTaskStart(int isStart);
#endif
