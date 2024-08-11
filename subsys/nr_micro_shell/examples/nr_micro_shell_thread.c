/**
 * @file      nr_micro_shell_thread.c
 * @author    Nrush
 * @version   V0.1
 * @date      28 Oct 2019
 * *****************************************************************************
 * @attention
 * 
 * MIT License
 * 
 * Copyright (C) 2019 Nrush. or its affiliates.  All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Includes ------------------------------------------------------------------*/
#include "nr_micro_shell.h"
#include <puppy.h>
#include <puppy/posix/pthread.h>

void *shell_thread_entry(void *parm)
{
    shell_init();
    pthread_setname_np(pthread_self(), "shell");
    while(1)
    {
        char c = p_hw_cons_getc();
        if(c)
        {
            shell(c);
        }
    }
}
char *shell_stack[4096];

static void shell_init_fn(void)
{
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setstackaddr(&attr, shell_stack);
   pthread_attr_setstacksize(&attr, 4096);
   pthread_create(NULL, &attr, shell_thread_entry, NULL);
}
P_INIT_FUNC(shell_init_fn);

/******************* (C) COPYRIGHT 2019 Nrush *****END OF FILE*****************/
