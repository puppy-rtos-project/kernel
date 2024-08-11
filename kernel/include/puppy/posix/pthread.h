/*
 * Copyright (c) 2022-2023, The Puppy RTOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PUPPY_INC_POSIX_PTHREAD_H__
#define PUPPY_INC_POSIX_PTHREAD_H__

#include <puppy/default_config.h>    /* Default settings */
#include <puppy/toolchan.h>  /* Compiler settings, noreturn_function */

#include <stdint.h>          /* C99 fixed width integer types */
#include <stdbool.h>         /* C99 boolean types */
#include <time.h>            /* Needed for struct timespec */

#ifndef _POSIX_THREADS
#define _POSIX_THREADS
#endif

#ifndef _POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_ATTR_STACKSIZE
#endif

/* Values for the process shared (pshared) attribute */

#define PTHREAD_PROCESS_PRIVATE       0
#define PTHREAD_PROCESS_SHARED        1

/* Valid ranges for the pthread stacksize attribute */

#define PTHREAD_STACK_MIN             P_CONFIG_PTHREAD_STACK_MIN
#define PTHREAD_STACK_DEFAULT         P_CONFIG_PTHREAD_STACK_DEFAULT

/* Values for the pthread inheritsched attribute */

#define PTHREAD_INHERIT_SCHED         0
#define PTHREAD_EXPLICIT_SCHED        1

/* Detach state  */

#define PTHREAD_CREATE_JOINABLE       0
#define PTHREAD_CREATE_DETACHED       1

/* Default priority */

#define PTHREAD_DEFAULT_PRIORITY      SCHED_PRIORITY_DEFAULT

/* Cancellation states used by pthread_setcancelstate() */

#define PTHREAD_CANCEL_ENABLE         (0)
#define PTHREAD_CANCEL_DISABLE        (1)

/* Cancellation types used by pthread_setcanceltype() */

#define PTHREAD_CANCEL_DEFERRED       (0)
#define PTHREAD_CANCEL_ASYNCHRONOUS   (1)

/* Thread return value when a pthread is canceled */

#define PTHREAD_CANCELED              ((void*)-1)

/* Used to initialize a pthread_once_t */

#define PTHREAD_ONCE_INIT             (false)

/* This is returned by pthread_barrier_wait.  It must not match any errno
 * in errno.h
 */

#define PTHREAD_BARRIER_SERIAL_THREAD 0x1000

#ifdef __cplusplus
extern "C"
{
#endif

/* pthread-specific types */

#ifndef __PTHREAD_KEY_T_DEFINED
typedef int pthread_key_t;
#  define __PTHREAD_KEY_T_DEFINED 1
#endif

#ifndef __PTHREAD_ADDR_T_DEFINED
typedef void *pthread_addr_t;
#  define __PTHREAD_ADDR_T_DEFINED 1
#endif

typedef int cpu_set_t;

typedef pthread_addr_t (*pthread_startroutine_t)(pthread_addr_t);
typedef pthread_startroutine_t pthread_func_t;

typedef void (*pthread_trampoline_t)(pthread_startroutine_t, pthread_addr_t);

struct pthread_attr_s
{
  uint8_t priority;            /* Priority of the pthread */
  uint8_t policy;              /* Pthread scheduler policy */
  uint8_t inheritsched;        /* Inherit parent priority/policy? */
  uint8_t detachstate;         /* Initialize to the detach state */
#ifdef CONFIG_SCHED_SPORADIC
  uint8_t low_priority;        /* Low scheduling priority */
  uint8_t max_repl;            /* Maximum pending replenishments */
#endif

#if P_CPU_NR > 1
  cpu_set_t affinity;          /* Set of permitted CPUs for the thread */
#endif

  void  *stackaddr;            /* Address of memory to be used as stack */
  size_t stacksize;            /* Size of the stack allocated for the pthread */

#ifdef CONFIG_SCHED_SPORADIC
  struct timespec repl_period; /* Replenishment period */
  struct timespec budget;      /* Initial budget */
#endif
};

#ifndef __PTHREAD_ATTR_T_DEFINED
typedef struct pthread_attr_s pthread_attr_t;
#  define __PTHREAD_ATTR_T_DEFINED 1
#endif

#ifndef __PTHREAD_T_DEFINED
typedef void *pthread_t;
#  define __PTHREAD_T_DEFINED 1
#endif

#ifndef __PTHREAD_ONCE_T_DEFINED
typedef bool pthread_once_t;
#  define __PTHREAD_ONCE_T_DEFINED 1
#endif

#if defined(CONFIG_PTHREAD_CLEANUP_STACKSIZE) && CONFIG_PTHREAD_CLEANUP_STACKSIZE > 0
/* This type describes the pthread cleanup callback (non-standard) */

typedef void (*pthread_cleanup_t)(void *arg);
#endif

/* Forward references */

struct sched_param; /* Defined in sched.h */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Initializes a thread attributes object (attr) with default values
 * for all of the individual attributes used by a given implementation.
 */

int pthread_attr_init(pthread_attr_t *attr);

/* An attributes object can be deleted when it is no longer needed. */

int pthread_attr_destroy(pthread_attr_t *attr);

/* Set or obtain the default scheduling algorithm */

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int pthread_attr_getschedpolicy(const pthread_attr_t *attr,
                                int *policy);
int pthread_attr_setinheritsched(pthread_attr_t *attr,
                                 int inheritsched);
int pthread_attr_getinheritsched(const pthread_attr_t *attr,
                                 int *inheritsched);
int pthread_attr_getdetachstate(const pthread_attr_t *attr,
                                int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr,
                                int detachstate);

/* Set or obtain the default stack size */

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr);
int pthread_attr_getstackaddr(const pthread_attr_t *attr,
                              void **stackaddr);

/* Set or obtain the default stack size */

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr,
                              size_t *stacksize);

/* Set or obtain stack address and size attributes */

int pthread_attr_setstack(pthread_attr_t *attr,
                          void *stackaddr, size_t stacksize);
int pthread_attr_getstack(const pthread_attr_t *attr,
                          void **stackaddr, size_t *stacksize);

/* Set or get the name of a thread */

int pthread_setname_np(pthread_t thread, const char *name);
int pthread_getname_np(pthread_t thread, char *name, size_t len);

/* Get run-time stack address and size */

void *pthread_get_stackaddr_np(pthread_t thread);
ssize_t pthread_get_stacksize_np(pthread_t thread);

/* To create a thread object and runnable thread, a routine must be specified
 * as the new thread's start routine.  An argument may be passed to this
 * routine, as an untyped address; an untyped address may also be returned as
 * the routine's value.  An attributes object may be used to specify details
 * about the kind of thread being created.
 */

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   pthread_startroutine_t startroutine, pthread_addr_t arg);

/* A thread object may be "detached" to specify that the return value and
 * completion status will not be requested.
 */

int pthread_detach(pthread_t thread);

/* A thread may terminate it's own execution or the execution of another
 * thread.
 */

void pthread_exit(pthread_addr_t value);
int  pthread_cancel(pthread_t thread);
int  pthread_setcancelstate(int state, int *oldstate);
int  pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);

/* A thread can await termination of another thread and retrieve the return
 * value of the thread.
 */

int pthread_join(pthread_t thread, pthread_addr_t *value);

/* A thread may tell the scheduler that its processor
 * can be made available.
 */

void pthread_yield(void);

/* A thread may obtain a copy of its own thread handle. */

pthread_t pthread_self(void);
#define pthread_gettid_np(thread) ((pid_t)(thread))

/* Compare two thread IDs. */

#define pthread_equal(t1,t2) ((t1) == (t2))

/* Pthread initialization */

int pthread_once(pthread_once_t *once_control,
                 void (*init_routine)(void));

int pthread_atfork(void (*prepare)(void),
                   void (*parent)(void),
                   void (*child)(void));

#ifdef __cplusplus
}
#endif

#endif 
