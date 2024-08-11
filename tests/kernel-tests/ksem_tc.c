/*
 * Copyright (c) 2022-2023, The Puppy RTOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include <puppy.h>
#include <puppy/posix/pthread.h>

#define KLOG_TAG  "ksem_tc"
#define KLOG_LVL   KLOG_LOG
#include <puppy/klog.h>

static struct _sem_obj sem;
volatile int _g_cnt = 0;

static void *t1_thread_entry(void *parm)
{
    while(1)
    {
        p_sem_wait(&sem);
        KLOG_D("t1:waited sem %d", ++_g_cnt);
        if (_g_cnt == 10)
        {
            P_TC_PASS();
            
            pthread_exit(0);
            return 0;
        }
    }
}

static void *t2_thread_entry(void *parm)
{
    int cnt = 0;
    while(cnt < 10)
    {
        p_sem_post(&sem);
        KLOG_D("main:post sem %d", ++cnt);
        p_thread_sleep(10);
    }
    
    pthread_exit(0);
}

char *pth1_stack[4096];
char *pth2_stack[4096];

void test_sem_api(void)
{
    pthread_t th1, th2;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 4096);

    _g_cnt = 0;
    p_sem_init(&sem, "sem", 0, 1);
    pthread_attr_setstackaddr(&attr, pth1_stack);
    pthread_create(&th1, &attr, t1_thread_entry, NULL);

    pthread_attr_setstackaddr(&attr, pth2_stack);
    pthread_create(&th2, &attr, t2_thread_entry, NULL);
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    P_TC_PASS();
}
P_TC_FUNC(test_sem_api, kernel.sem.tc);