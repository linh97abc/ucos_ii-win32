#ifndef __RTOS_THREAD_H__
#define __RTOS_THREAD_H__
#include <functional>
#include <stdint.h>

#include <ucos_ii.h>

namespace rtos
{
    struct Thread_prop
    {
        OS_STK *pbos;
        INT32U stk_size;
        INT8U id;
        static OS_FLAG_GRP *pgrp_task_running;
    };

    class Thread
    {
    private:
        class Callback
        {
        public:
            std::function<void()> entry;
            Thread_prop *prop;
        };

    public:
        enum
        {
            OS_STK_SIZE = 1024
        };

        enum class Priority
        {
            HIGH,
            NORMAL,
            LOW
        };

        Thread(Priority prio = Priority::NORMAL, void *stack_mem = nullptr, uint32_t stack_size = 0);

        template <typename... T>
        int start(void (*fn)(T... args), T... args)
        {
            static Callback cb;
            cb.entry = [fn, args...]()
            {
                fn(args...);
            };
            cb.prop = &this->prop;

            return on_start((void *)&cb);
        }

        template <typename T>
        int start(T fn)
        {
            static Callback cb;
            cb.entry = fn;
            cb.prop = &this->prop;

            return on_start((void *)&cb);
        }

        void join();
        int suspend();
        int resume();
        int stop();
        ~Thread() = default;

        static void Init();

    private:
        int on_start(void *cb);
        static void RootTask();

        Thread_prop prop;
    };

}

#endif
