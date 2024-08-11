/*
 * Copyright (c) 2022-2023, The Puppy RTOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <puppy.h>
#include <puppy/kobj.h>
#include <string.h>

#define KLOG_TAG  "thread"
#define KLOG_LVL   KLOG_WARNING
#include <puppy/klog.h>

#define P_THREAD_SLICE_DEFAULT 10

static p_list_t thread_timeout_list = P_LIST_STATIC_INIT(&thread_timeout_list);
static void timeout_insert(struct timeout *timeout);
static int timeout_remove(struct timeout *timeout);

void *p_thread_get_archdata(p_obj_t obj)
{
    struct _thread_obj *thread = obj;
    return thread->arch_data;
}
void p_thread_entry(void (*entry)(void *parameter), void *param)
{
    KLOG_D("p_thread_entry enter...");
    if (entry)
    {
        entry(param);
    }
    
    KLOG_D("p_thread_entry exit...");
    p_thread_abort(p_thread_self());
    while (1);
}
static void _p_thread_cleanup(p_obj_t obj)
{
    struct _thread_obj *thread = obj;
    // p_free(thread->stack_addr);
    p_obj_deinit(thread);
}
void p_thread_init(p_obj_t obj, const char *name,
                                void (*entry)(void *param),
                                void    *param,
                                void    *stack_addr,
                                uint32_t stack_size,
                                uint8_t  prio,
                                uint8_t bindcpu)
{
    struct _thread_obj *thread = obj;

    p_obj_init(&thread->kobj, name, P_OBJ_TYPE_THREAD | P_OBJ_TYPE_STATIC, 0);
    
    thread->entry = entry;
    thread->param = param;
    thread->stack_addr = stack_addr;
    thread->stack_size = stack_size;
    thread->prio = prio;
    thread->slice_ticks = P_THREAD_SLICE_DEFAULT;
    thread->state = P_THREAD_STATE_INIT;
    thread->mode = 0;
    thread->cleanup = _p_thread_cleanup;
    thread->kernel_stack = 0;
    thread->bindcpu = bindcpu;
    thread->oncpu = CPU_NA;
    
    KLOG_D("thread_init -->[%s], entry:0x%x, stack_addr:0x%x, stack_size:%d ",
                               name, entry, stack_addr, stack_size);
    
    memset(stack_addr,0x23, stack_size);

    thread->arch_data = arch_new_thread(p_thread_entry, entry, param, stack_addr, stack_size);
}

p_obj_t p_thread_self(void)
{
    return p_cpu_self()->curr_thread;
}

p_obj_t p_thread_next(void)
{
    return p_cpu_self()->next_thread;
}

const char *p_thread_self_name(void)
{
    KLOG_ASSERT(p_cpu_self()->curr_thread != NULL);
    KLOG_ASSERT(p_obj_get_type(p_cpu_self()->curr_thread) == P_OBJ_TYPE_THREAD);
    return p_cpu_self()->curr_thread->kobj.name;
}

int p_thread_abort(p_obj_t obj)
{
    struct _thread_obj *_thread = obj;
    p_base_t key = arch_irq_lock();
    KLOG_ASSERT(p_obj_get_type(_thread) == P_OBJ_TYPE_THREAD);

    _thread->state = P_THREAD_STATE_DEAD;
    timeout_remove(&_thread->timeout);
    p_thread_dead_add(_thread);
    p_sched();
    
    arch_irq_unlock(key);
    return 0;
}

int p_thread_yield(void)
{
    struct _thread_obj *_thread = p_cpu_self()->curr_thread;
    p_base_t key = arch_irq_lock();
    
    KLOG_ASSERT(p_cpu_self()->curr_thread != NULL);
    KLOG_ASSERT(p_obj_get_type(_thread) == P_OBJ_TYPE_THREAD);
    KLOG_ASSERT(_thread->state == P_THREAD_STATE_RUN);

    p_sched_ready_insert(_thread);
    p_sched();
    
    arch_irq_unlock(key);
    return 0;
}

void sleep_timeout_fn(p_obj_t obj, void *param)
{
    struct _thread_obj *_thread = obj;
    _thread->errno = P_ETIMEOUT;
    KLOG_ASSERT(obj != NULL);
    p_sched_ready_insert(_thread);
    p_sched();
}

int p_thread_set_timeout(p_tick_t timeout, p_timeout_fn func, void *param)
{
    struct _thread_obj *_thread = p_cpu_self()->curr_thread;
    
    KLOG_ASSERT(p_obj_get_type(_thread) == P_OBJ_TYPE_THREAD);
    KLOG_ASSERT(_thread->state == P_THREAD_STATE_RUN);
    KLOG_ASSERT(p_node_is_linked(&_thread->timeout.node) == false);
    
    /* check timeout node */
    if(p_node_is_linked(&_thread->timeout.node))
    {
        return -P_EINVAL;
    }

    _thread->timeout.tick = p_tick_get() + timeout;
    _thread->timeout.func = func;
    _thread->timeout.param = param;
    timeout_insert(&_thread->timeout);
    return 0;
}

int p_thread_sleep(p_tick_t tick)
{
    struct _thread_obj *_thread = p_cpu_self()->curr_thread;
    int err = P_EOK;
    p_base_t key = arch_irq_lock();

    KLOG_ASSERT(p_obj_get_type(_thread) == P_OBJ_TYPE_THREAD);
    KLOG_ASSERT(_thread->state == P_THREAD_STATE_RUN);
    
    p_thread_set_timeout(tick, sleep_timeout_fn, NULL);

    _thread->state = P_THREAD_STATE_SLEEP;

    err = p_sched();
    if(err == -P_ETIMEOUT)
    {
        err = P_EOK;
        p_set_errno(err);
    }

    arch_irq_unlock(key);
    return err;
}

int p_thread_wakeup(p_obj_t obj)
{
    struct _thread_obj *_thread = obj;
    int err = P_EOK;
    p_base_t key = arch_irq_lock();
    KLOG_ASSERT(obj != NULL);
    KLOG_ASSERT(p_obj_get_type(_thread) == P_OBJ_TYPE_THREAD);

    p_sched_ready_insert(_thread);

    arch_irq_unlock(key);
    return err;
}

int p_thread_block(p_obj_t obj)
{
    struct _thread_obj *_thread = obj;
    int err = P_EOK;
    p_base_t key = arch_irq_lock();
    KLOG_ASSERT(p_obj_get_type(_thread) == P_OBJ_TYPE_THREAD);
    KLOG_ASSERT(_thread->state == P_THREAD_STATE_RUN);

    if (_thread->state != P_THREAD_STATE_RUN)
    {
        err = -P_EINVAL;
        goto _exit;
    }
    _thread->state = P_THREAD_STATE_BLOCK;

_exit:
    arch_irq_unlock(key);
    return err;
}

int p_thread_start(p_obj_t obj)
{
    KLOG_ASSERT(obj != NULL);
    KLOG_ASSERT(p_obj_get_type(obj) == P_OBJ_TYPE_THREAD);
    
    p_sched_ready_insert(obj);
    p_sched();
    return 0;
}


/******** thread timeout ********/
static struct timeout *_next_timeout(void)
{
    struct timeout *timeout;
    p_base_t key = arch_irq_lock();

    if (!p_list_is_empty(&thread_timeout_list))
    {
        timeout = p_list_entry(thread_timeout_list.head,
                             struct timeout, node);
        arch_irq_unlock(key);
        return timeout;
    }

    arch_irq_unlock(key);

    return NULL;
}

static void timeout_insert(struct timeout *timeout)
{
    struct timeout *_timeout = NULL, *temp_timeout;
    p_base_t key = arch_irq_lock();
    p_node_t *node;
    
    p_list_for_each_node(&thread_timeout_list, node)
    {
        temp_timeout = p_list_entry(node, struct timeout, node);
        if (temp_timeout->tick > timeout->tick)
        {
            /* find out insert node */
            _timeout = temp_timeout;
            break;
        }
    }

    if (_timeout)
    {
        p_list_insert(&_timeout->node, &timeout->node);
    }
    else
    {
        p_list_append(&thread_timeout_list, &timeout->node);
    }

    arch_irq_unlock(key);
}
static int timeout_remove(struct timeout *timeout)
{
    p_base_t key = arch_irq_lock();

    if (timeout->node.next != NULL)
    {
        p_list_remove(&timeout->node);
    }

    arch_irq_unlock(key);
    return 0;
}

void thread_timeout_cb(p_base_t tick)
{
    struct timeout *timeout;
    struct _thread_obj *_thread;
_next:
    timeout = _next_timeout();
    if(timeout && timeout->tick <= tick)
    {
        _thread = p_list_entry(timeout,
                            struct _thread_obj, timeout);
        
        KLOG_ASSERT(p_obj_get_type(_thread) == P_OBJ_TYPE_THREAD);
        /* timeout ok */
        p_list_remove(&timeout->node);
        /* todo */
        if (timeout->func)
        {
            timeout->func(_thread, timeout->param);
        }
        goto _next;
    }
}

void p_thread_dead_add(p_obj_t tid)
{
    struct _thread_obj *th = tid;
    p_base_t key = arch_irq_lock();
    p_list_append(&p_cpu_self()->dead_queue, &th->tnode);
    arch_irq_unlock(key);
}

void p_thread_dead_clean(void)
{
    p_node_t *node, *node_s;
    p_base_t key = arch_irq_lock();
    p_list_for_each_node_safe(&p_cpu_self()->dead_queue, node, node_s)
    {
        struct _thread_obj *_thread = p_list_entry(node, struct _thread_obj, tnode);
        p_list_remove(node);
        arch_irq_unlock(key);
        
        if (_thread->cleanup)
        {
            _thread->cleanup(_thread); /* todo: if other thread blocked in free func */
        }
        key = arch_irq_lock();
    }
    arch_irq_unlock(key);
}
