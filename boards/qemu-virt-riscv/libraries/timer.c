#include "puppy_core.h"
#include "platform.h"
#include "riscv.h"

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ / 100

static uint64_t next_time = 0;

/* load timer interval(in ticks) for next timer interrupt.*/
uint32_t timer_load(int interval)
{
    /* each CPU has a separate source of timer interrupts. */
    int id = r_mhartid();
    uint64_t cur_time = *(uint64_t*)CLINT_MTIME;
    uint32_t inc_time = cur_time - next_time;

    if (inc_time > interval)
    {
        inc_time = 0;
    }

    /* 减少误差. */
    next_time = cur_time + interval - inc_time;

    *(uint64_t*)CLINT_MTIMECMP(id) = next_time;
    return inc_time;
}

void timer_init()
{
    /*
     * On reset, mtime is cleared to zero, but the mtimecmp registers 
     * are not reset. So we have to init the mtimecmp manually.
     */
    timer_load(TIMER_INTERVAL);

    /* enable machine-mode timer interrupts. */
    w_mie(r_mie() | MIE_MTIE);

    /* enable machine-mode global interrupts. */
    // w_mstatus(r_mstatus() | MSTATUS_MIE);
}

void timer_handler() 
{
    uint32_t inc = 0;
    inc = timer_load(TIMER_INTERVAL);
    // p_tick_inc(1, inc);
}