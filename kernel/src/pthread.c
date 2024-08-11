/**
 * @file pthread.c
 * @author Henson
 * @brief pthread api
 * @version 0.1
 * @date 2023-08-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <puppy.h>
#include <puppy/kobj.h>
#include <puppy/posix/unistd.h>
#include <puppy/posix/pthread.h>
#include <string.h>

#define KLOG_TAG  "pthread"
#define KLOG_LVL   KLOG_WARNING
#include <puppy/klog.h>

const pthread_attr_t pthread_default_attr =
{
    P_SCHED_PRIO_DEFAULT,
    0, 0,     
    PTHREAD_CREATE_JOINABLE,    /* detach state */
#if P_CPU_NR > 1
    CPU_NA,
#endif
    0,
    P_CONFIG_PTHREAD_STACK_DEFAULT, 
};

struct _pthread_data
{
    pthread_attr_t attr;
    struct _thread_obj tid;
    void *alloc_stack;

    pthread_func_t thread_entry;
    void *thread_parameter;

    /* return value */
    void *return_value;

    /* semaphore for joinable thread */
    struct _sem_obj joinable_sem;

    /* cancel state and type */
    uint8_t cancelstate;
    volatile uint8_t canceltype;
    volatile uint8_t canceled;

    // _pthread_cleanup_t *cleanup;
    void** tls; /* thread-local storage area */
};
typedef struct _pthread_data _pthread_data_t;

pthread_t _pthread_data_create(const pthread_attr_t *attr)
{
    _pthread_data_t *ptd = NULL;

    KLOG_ASSERT(attr->stackaddr != NULL);
    // create pthread date from stack top.
    KLOG_D("ptd attr.stackaddr:0x%x, size:%d", attr->stackaddr, attr->stacksize);
    ptd = (_pthread_data_t*)P_ALIGN_DOWN((p_ubase_t)(attr->stackaddr + (attr->stacksize - sizeof(_pthread_data_t))) , P_ALIGN_SIZE);
    KLOG_D("ptd created at addr:0x%x", ptd);
    if (!ptd) return NULL;

    memset(ptd, 0x0, sizeof(_pthread_data_t));
    ptd->canceled = 0;
    ptd->cancelstate = PTHREAD_CANCEL_DISABLE;
    ptd->canceltype = PTHREAD_CANCEL_DEFERRED;

    return ptd;
}
void _pthread_data_destroy(_pthread_data_t *ptd)
{
    // p_free(ptd);
}

static void _pthread_cleanup(p_obj_t obj)
{
    _pthread_data_t *ptd;
    ptd = p_container_of(obj, _pthread_data_t, tid);
    p_sem_post(&ptd->joinable_sem);
}

int pthread_attr_init(pthread_attr_t *attr)
{
    *attr = pthread_default_attr;
    return 0;
}
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr)
{
    attr->stackaddr = stackaddr;
    return 0;
}
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    attr->stacksize = stacksize;
    return 0;
}
/**
 * @brief Create a new thread
 * 
 *        The pthread_create() function starts a new thread in the calling
 *        process.  The new thread starts execution by invoking
 *        start_routine(); arg is passed as the sole argument of
 *        start_routine().
 * 
 *        The new thread terminates in one of the following ways:
 *        •  It calls pthread_exit(), specifying an exit status value that
 *           is available to another thread in the same process that calls
 *           pthread_join().
 *        •  It returns from start_routine().  This is equivalent to
 *           calling pthread_exit() with the value supplied in the return
 *           statement.
 * 
 * @param thread Buffer to store new thread ID, this identifier is used to refer
 *               to the thread in subsequent calls to other pthreads functions. 
 * 
 * @param attr   The attr argument points to a pthread_attr_t structure whose
 *               contents are used at thread creation time to determine attributes
 *               for the new thread; this structure is initialized using
 *               pthread_attr_init() and related functions.  If attr is NULL,
 *               then the thread is created with default attributes.
 * 
 * @param start_routine Function to run when the new thread executes.
 * @param arg arg is passed as the sole argument of start_routine().
 * @return On success, pthread_create() returns 0; on error, it returns an
 *             error number, and the contents of *thread are undefined.
 */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   pthread_startroutine_t start_routine, pthread_addr_t arg)
{
    int ret = 0;
    _pthread_data_t *ptd;
    void *stack = NULL;
    /* allocate posix thread data */
    ptd = _pthread_data_create(attr);
    if (ptd == NULL)
    {
        ret = -P_ENOMEM;
        goto _exit;
    }

    if (attr != NULL)
    {
        ptd->attr = *attr;
    }
    else
    {
        /* use default attribute */
        ptd->attr = pthread_default_attr;
    }
    if (ptd->attr.detachstate == PTHREAD_CREATE_JOINABLE)
    {
        p_sem_init(&ptd->joinable_sem, "sem", 0, 1);
    }
     /* set parameter */
    ptd->thread_entry = start_routine;
    ptd->thread_parameter = arg;

    /* stack */
    if (ptd->attr.stackaddr == 0)
    {
        // stack = (void *)p_malloc(ptd->attr.stacksize);
        KLOG_E("attr.stackaddr must be set!");
        if (stack == NULL)
        {
            ret = -P_ENOMEM;
            goto _exit;
        }

        ptd->alloc_stack = stack;
    }
    else
    {
        stack = (void *)(ptd->attr.stackaddr);
    }
    /* initial this pthread to system */
    p_thread_init(&ptd->tid, "pth", (void (*)(void *))start_routine, arg,
                       stack, ptd->attr.stacksize - P_ALIGN(sizeof(_pthread_data_t), P_ALIGN_SIZE),
                       ptd->attr.priority, CPU_NA);
    ptd->tid.cleanup = _pthread_cleanup;
    if(thread) *thread = ptd;
        /* start thread */
    if (p_thread_start(&ptd->tid) == 0)
        return 0;
_exit:

    return ret;
}
/**
 * @brief join with a terminated thread
 * 
 *        The pthread_join() function waits for the thread specified by
 *        thread to terminate.  If that thread has already terminated, then
 *        pthread_join() returns immediately.  The thread specified by
 *        thread must be joinable.
 * 
 * @param thread Thread ID is created by pthread_create().
 * @param value the value passed to pthread_exit() by the terminating thread. 
 * @return On success, pthread_join() returns 0; on error, it returns a
 *             error number.
 */
int pthread_join(pthread_t thread, void **value)
{
    _pthread_data_t *ptd;
    int result;

    ptd = thread;

    if (ptd == NULL)
    {
        return -1; /* invalid pthread id */
    }

    if (ptd->attr.detachstate == PTHREAD_CREATE_DETACHED)
    {
        return -1; /* join on a detached pthread */
    }

    result = p_sem_wait(&ptd->joinable_sem);
    if (result == 0)
    {
        /* get return value */
        if (value != NULL)
            *value = ptd->return_value;

        if (ptd->alloc_stack)
        {
            // p_free(ptd->alloc_stack);
        }
        p_obj_deinit(&ptd->tid);
        p_obj_deinit(&ptd->joinable_sem);

        /* destroy this pthread */
        _pthread_data_destroy(ptd);
    }
    else
    {
        return -1;
    }

    return 0;
}

/**
 * @brief terminate calling thread
 * 
 *        The pthread_exit() function terminates the calling thread and
 *        returns a value via value.
 * 
 * @param value value passed to thread that calls pthread_join().
 */
void pthread_exit(void *value)
{
    _pthread_data_t *ptd;
    ptd = p_container_of(p_thread_self(), _pthread_data_t, tid);
    ptd->return_value = value;
}
/**
 * @brief obtain ID of the calling thread
 * 
 *        The pthread_self() function returns the ID of the calling thread.
 *        This is the same value that is returned in *thread in the
 *        pthread_create() call that created this thread.
 * 
 * @return the calling thread's ID.
 */
pthread_t pthread_self(void)
{
    _pthread_data_t *ptd;
    ptd = p_container_of(p_thread_self(), _pthread_data_t, tid);
    return ptd;
}

/**
 * @brief set the name of a thread
 * 
 * @param thread argument specifies the thread whose name is to be changed.
 * @param name name specifies the new name.
 * @return On success, these functions return 0; on error, they return a
 *             nonzero error number.
 */
int pthread_setname_np(pthread_t thread, const char *name)
{
    _pthread_data_t *ptd = thread;
    ptd->tid.kobj.name = name;
    return 0;
}

unsigned sleep(unsigned int __seconds)
{   
    p_thread_sleep(p_tick_persec() * __seconds);
    return 0;
}
