#include <iostream>
#include <functional>
#include <stdint.h>

#include <ucos_ii.h>

#include "Thread.h"

namespace rtos
{
    OS_FLAG_GRP *Thread_prop::pgrp_task_running;

    Thread::Thread(Thread::Priority prio, void *stack_mem, uint32_t stack_size)
    {
        if (stack_mem)
        {
            this->prop.pbos = (OS_STK *)stack_mem;
            this->prop.stk_size = stack_size;
        }
        else
        {
            this->prop.pbos = nullptr;
            this->prop.stk_size = OS_STK_SIZE;
        }

        switch (prio)
        {
        case Priority::HIGH:
            this->prop.id = 2;
            /* code */
            break;
        case Priority::NORMAL:
            this->prop.id = 8;
            /* code */
            break;

        default:
            this->prop.id = 15;
            break;
        }

        INT8U err;
        OSFlagPost(this->prop.pgrp_task_running, 1u << this->prop.id, OS_FLAG_CLR, &err);
    }

    int Thread::on_start(void *cb)
    {
        OS_STK *ptos;
        OS_STK *pbos = this->prop.pbos;

        INT8U err = -1;
        INT8U prio_tmp = this->prop.id;

        static OS_STK stk_mem_region[OS_LOWEST_PRIO][OS_STK_SIZE];

        while ((err != OS_ERR_NONE) && (prio_tmp < OS_LOWEST_PRIO))
        {
            if (!this->prop.pbos)
            {
                pbos = &stk_mem_region[prio_tmp][0];
            }
            ptos = &pbos[this->prop.stk_size - 1];
            this->prop.id = prio_tmp;

            err = OSTaskCreateExt(
                [](void *pdata)
                {
                    Callback *t = (Callback *)pdata;
                    t->entry();
                    INT8U err;
                    OSFlagPost(t->prop->pgrp_task_running, 1u << t->prop->id, OS_FLAG_CLR, &err);
                    OSTaskDel(t->prop->id);
                },
                (void *)cb,
                ptos, this->prop.id, this->prop.id, pbos, this->prop.stk_size, nullptr, 0);

            prio_tmp++;
        }

        if (err == OS_ERR_NONE)
        {
            OSFlagPost(this->prop.pgrp_task_running, 1u << this->prop.id, OS_FLAG_SET, &err);
        }

        return 0;
    }

    void Thread::join()
    {
        INT8U err;
        OSFlagPend(this->prop.pgrp_task_running, 1u << this->prop.id, OS_FLAG_WAIT_CLR_ALL, 0, &err);
        // OSFlagPend(this->prop.pgrp_task_running, 0, OS_FLAG_WAIT_SET_ALL, 0, &err);
        // OSFlagPend(this->prop.pgrp_task_running, 0, OS_FLAG_WAIT_CLR_ALL, 0, &err);
        // OSFlagPend(this->prop.pgrp_task_running, 0, OS_FLAG_WAIT_SET_ALL, 0, &err);
        // OSFlagPend(this->prop.pgrp_task_running, 0, OS_FLAG_WAIT_CLR_ALL, 0, &err);
    }

    void Thread::Init()
    {
        static OS_STK stk_mem[OS_STK_SIZE];
        INT8U err;

        OSInit();
        Thread_prop::pgrp_task_running = OSFlagCreate(0, &err);
        OSTaskCreateExt(
            [](void *pdata)
            {
                Thread::RootTask();
                OSTaskDel(1);
            },
            nullptr,
            &stk_mem[OS_STK_SIZE - 1], 1, 1, stk_mem, OS_STK_SIZE, nullptr, 0);
        OSStart();
    }

    int Thread::suspend()
    {
        return OSTaskSuspend(this->prop.id);
    }

    int Thread::resume()
    {
        return OSTaskResume(this->prop.id);
    }

    int Thread::stop()
    {
        return OSTaskDel(this->prop.id);
    }
}

///////////////////////////
void test(int a, int b)
{
    std::cout << __func__ << " " << a << " " << b << std::endl;
}

int main()
{
    rtos::Thread::Init();

    return 0;
}

void test_fn()
{
    std::cout << __func__ << " " << __LINE__ << std::endl;

    rtos::Thread t1, t2;

    t1.start([]()
             { std::cout << __func__ << " " << __LINE__ << std::endl; });

    t2.start([]()
             { std::cout << __func__ << " " << __LINE__ << std::endl; });
}

OS_STK stk_mem[1024];
void rtos::Thread::RootTask()
{
    // rtos::Thread t(rtos::Thread::Priority::LOW);
    rtos::Thread t;
    rtos::Thread t2;

    t2.start(
        []()
        {
            OSTimeDlyHMSM(0, 0, 1, 0);
            std::cout << __func__ << " " << __LINE__ << std::endl;
        });

    // t.start(test_fn);
    t.start(test, 13, 27);

    // t2.join();

    std::cout << __func__ << " " << __LINE__ << std::endl;
}