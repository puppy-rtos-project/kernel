/*
 * Copyright (c) 2022-2023, The Puppy RTOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <puppy.h>
#include <puppy/kobj.h>

#define KLOG_TAG  "init"
#define KLOG_LVL   KLOG_WARNING
#include <puppy/klog.h>

static struct _thread_obj _idle[P_CPU_NR];
p_align(P_ALIGN_SIZE)
static uint8_t _idle_thread_stack[P_CPU_NR][P_IDLE_THREAD_STACK_SIZE];


static void idle_thread_entry(void *parm)
{
    uint8_t cpu_id = p_cpu_self_id();
    while(1)
    {
        if (!p_list_is_empty(&p_cpu_index(cpu_id)->dead_queue))
        {
            p_thread_dead_clean();
        }
        else
        {
            __asm("wfi");
        }
    }
}

const P_SECTION_START_DEFINE(P_INIT_SECTION, _init_start);
const P_SECTION_END_DEFINE(P_INIT_SECTION, _init_end);
static void _init_fn_run(void)
{
    struct p_ex_fn *ptr_begin, *ptr_end;
    volatile struct p_ex_fn *init_fn;
    ptr_begin = (struct p_ex_fn *)P_SECTION_START_ADDR(_init_start);
    ptr_end = (struct p_ex_fn *)P_SECTION_END_ADDR(_init_end);
    for (init_fn = ptr_begin; init_fn < ptr_end;)
    {
        KLOG_D("init [%s] init...",  init_fn->name);
        (init_fn ++)->func();
    }
}

void p_show_version(void)
{
    printk("\n\nBuild Time: %s %s\n", __DATE__, __TIME__);
    printk("                           _         \n");
    printk("    ____   ____    _____  (_) _  __\n");
    printk("   / __ \\ / __ \\  / ___/ / / | |/_/\n");
    printk("  / /_/ // /_/ / (__  ) / /  >  <  \n");
    printk(" / .___/ \\____/ /____/ /_/  /_/|_|  \n");
    printk("/_/          Powered dy puppy-rtos\n");
}

p_weak void p_subcpu_start(void)
{
    ;
}

// #include <puppy/posix/pthread.h>
void puppy_init(void)
{
    puppy_board_init();
    p_cpu_init();
    p_show_version();
    _init_fn_run();
    for (uint8_t i = 0; i < P_CPU_NR; i++)
    {
        // pthread_attr_t attr;
        // pthread_attr_init(&attr);
        // attr.priority = P_THREAD_PRIO_MAX;
        // attr.affinity = 
        // pthread_attr_setstackaddr(&attr, _idle_thread_stack[i]);
        // pthread_attr_setstacksize(&attr, P_IDLE_THREAD_STACK_SIZE);
        // pthread_create(NULL, &attr, idle_thread_entry, NULL);

        p_thread_init(&_idle[i], "idle", idle_thread_entry, NULL,
                    _idle_thread_stack[i], P_IDLE_THREAD_STACK_SIZE,
                    P_THREAD_PRIO_MAX, i);
        p_thread_start(&_idle[i]);
    }
    p_subcpu_start();
}

void puppy_start(void)
{
    p_sched_unlock();
}
