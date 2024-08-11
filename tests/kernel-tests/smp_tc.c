/*
 * Copyright (c) 2022-2023, The Puppy RTOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include <puppy.h>
#include <puppy/posix/unistd.h>
#include <puppy/posix/sched.h>
#include <puppy/posix/pthread.h>

#define KLOG_TAG  "tc.ksmp"
#define KLOG_LVL   KLOG_LOG
#include <puppy/klog.h>

#define NUM_THREADS 20

static int end_exec;
static pthread_t threads[NUM_THREADS];
static int thread_ids[NUM_THREADS];

void *thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    KLOG_D("Thread %d is running on core %d", thread_id, sched_getcpu());
    // Do some computation here
    
    KLOG_D("Thread %d begin sleep on core %d", thread_id, sched_getcpu());
    sleep(1);
    
    KLOG_D("Thread %d end sleep on core %d", thread_id, sched_getcpu());
    pthread_exit(0);
    return NULL;
}

int smp_testcase(void)
{
    int i, rc;

    // Create threads
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        rc = pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
        if (rc) {
            KLOG_E("Error creating thread %d", i);
            return -1;
        }
    }

    // Wait for threads to finish
    for (i = 0; i < NUM_THREADS; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            KLOG_E("Error joining thread %d", i);
            return -1;
        }
    }
    KLOG_D("Test PASSED");

    return 0;
}

void test_smp(void)
{
    smp_testcase();
    P_TC_PASS();
}
// P_TC_FUNC(test_smp, kernel.smp.tc);