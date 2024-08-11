/*
 * Copyright (c) 2022-2023, The Puppy RTOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include <puppy.h>
#include <puppy/posix/unistd.h>
#include <puppy/posix/pthread.h>

static int end_exec;

static void *a_thread_func()
{
    int i;

    printk("Wait for 3 seconds for thread to finish execution:\n");
    for (i = 1; i < 4; i++) {
        printk("Waited (%d) second\n", i);
        sleep(1);
    }

    /* Indicate that the thread has ended execution. */
    end_exec = 1;

    pthread_exit(0);
    return NULL;
}

char *pth_stack[4096];

static int posix_testcase(void)
{
    pthread_t new_th;
    /* Initialize flag */
    end_exec = 0;

    /* Create a new thread. */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstackaddr(&attr, pth_stack);
    pthread_attr_setstacksize(&attr, 4096);
    if (pthread_create(&new_th, &attr, a_thread_func, NULL) != 0) {
        printk("Error creating thread\n");
        return -2;
    }

    /* Wait for thread to return */
    if (pthread_join(new_th, NULL) != 0) {
        printk("Error in pthread_join()\n");
        return -2;
    }

    if (end_exec == 0) {
        printk("Test FAILED: When using pthread_join(), "
               "main() did not wait for thread to finish "
               "execution before continuing.\n");
        return -1;
    }

    printk("Test PASSED\n");
    return 0;
}

void test_pthread_api(void)
{
    posix_testcase();
    P_TC_PASS();
}
P_TC_FUNC(test_pthread_api, kernel.pthread.tc);