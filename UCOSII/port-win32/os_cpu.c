/* Standard includes. */
#include <stdio.h>
#include <stdint.h>

/* Scheduler includes. */
#include <ucos_ii.h>
// typedef unsigned int OS_STK;
// #define _WIN32_WINNT 0x0400
#include <windows.h>
#include <winbase.h>

#ifdef __GNUC__
#include <mmsystem.h>
#else
#pragma comment(lib, "winmm.lib")
#endif

#define portTICK_PERIOD_MS 1000u / OS_TICKS_PER_SEC

/* The priorities at which the various components of the simulation execute. */
#define portSIMULATED_INTERRUPTS_THREAD_PRIORITY THREAD_PRIORITY_TIME_CRITICAL
#define portSIMULATED_TIMER_THREAD_PRIORITY THREAD_PRIORITY_HIGHEST
#define portTASK_THREAD_PRIORITY THREAD_PRIORITY_ABOVE_NORMAL

/*
 * Created as a high priority thread, this function uses a timer to simulate
 * a tick interrupt being generated on an embedded target.  In this Windows
 * environment the timer does not achieve anything approaching real time
 * performance though.
 */
static DWORD WINAPI thread_timer(LPVOID lpParameter);

/*
 * Process all the simulated interrupts - each represented by a bit in
 * ulPendingInterrupts variable.
 */
static void prvProcessSimulatedInterrupts(void);

/*
 * Exiting a critical section will cause the calling task to block on yield
 * event to wait for an interrupt to process if an interrupt was pended while
 * inside the critical section.  This variable protects against a recursive
 * attempt to obtain pvInterruptEventMutex if a critical section is used inside
 * an interrupt handler itself.
 */
volatile BOOLEAN xInsideInterrupt = FALSE;

/*
 * Called when the process exits to let Windows know the high timer resolution
 * is no longer required.
 */
static BOOL WINAPI prvEndProcess(DWORD dwCtrlType);

/*-----------------------------------------------------------*/

/* The WIN32 simulator runs each task in a thread.  The context switching is
managed by the threads, so the task stack does not have to be managed directly,
although the task stack is still used to hold an xThreadState structure this is
the only thing it will ever hold.  The structure indirectly maps the task handle
to a thread handle. */
typedef struct
{
    /* Handle of the thread that executes the task. */
    void *pvThread;
} ThreadState_t;

/* Simulated interrupts waiting to be processed.  This is a bit mask where each
bit represents one interrupt, so a maximum of 32 interrupts can be simulated. */
static volatile uint32_t ulPendingInterrupts = 0UL;

/* An event used to inform the simulated interrupt processing thread (a high
priority thread that simulated interrupt processing) that an interrupt is
pending. */
static void *pvInterruptEvent = NULL;

/* Mutex used to protect all the simulated interrupt variables that are accessed
by multiple threads. */
static void *pvInterruptEventMutex = NULL;

/*-----------------------------------------------------------*/
// void task1(void) {puts("task1");}
// void taskY(void) {puts("taskY");}

// static ThreadState_t s_ThreadSate = {task1, taskY};
static ThreadState_t *s_pThreadSate = NULL;
/*-----------------------------------------------------------*/

static DWORD WINAPI thread_timer(LPVOID lpParameter)
{
    unsigned int xMinimumWindowsBlockTime;
    TIMECAPS xTimeCaps;

    /* Set the timer resolution to the maximum possible. */
    if (timeGetDevCaps(&xTimeCaps, sizeof(xTimeCaps)) == MMSYSERR_NOERROR)
    {
        xMinimumWindowsBlockTime = xTimeCaps.wPeriodMin;
        timeBeginPeriod(xTimeCaps.wPeriodMin);

        /* Register an exit handler so the timeBeginPeriod() function can be
        matched with a timeEndPeriod() when the application exits. */
        SetConsoleCtrlHandler(prvEndProcess, TRUE);
    }
    else
    {
        xMinimumWindowsBlockTime = 20;
    }

    /* Just to prevent compiler warnings. */
    (void)lpParameter;

    // printf("xMinimumWindowsBlockTime: %d, portTICK_PERIOD_MS: %d\n", xMinimumWindowsBlockTime, portTICK_PERIOD_MS);
    for (;;)
    {
        /* Wait until the timer expires and we can access the simulated interrupt
        variables.  *NOTE* this is not a 'real time' way of generating tick
        events as the next wake time should be relative to the previous wake
        time, not the time that Sleep() is called.  It is done this way to
        prevent overruns in this very non real time simulated/emulated
        environment. */
        if (portTICK_PERIOD_MS < xMinimumWindowsBlockTime)
        {
            Sleep(xMinimumWindowsBlockTime);
        }
        else
        {
            Sleep(portTICK_PERIOD_MS);
        }

        /* Can't proceed if in a critical section as pvInterruptEventMutex won't
        be available. */
        WaitForSingleObject(pvInterruptEventMutex, INFINITE);

        /* The timer has expired, generate the simulated tick event. */
        ulPendingInterrupts = TRUE;

        /* The interrupt is now pending - notify the simulated interrupt
        handler thread.  Must be outside of a critical section to get here so
        the handler thread can execute immediately pvInterruptEventMutex is
        released. */
        SetEvent(pvInterruptEvent);

        /* Give back the mutex so the simulated interrupt handler unblocks
        and can access the interrupt handler variables. */
        ReleaseMutex(pvInterruptEventMutex);
    }

#ifdef __GNUC__
    /* Should never reach here - MingW complains if you leave this line out,
    MSVC complains if you put it in. */
    return 0;
#endif
}
/*-----------------------------------------------------------*/

static BOOL WINAPI prvEndProcess(DWORD dwCtrlType)
{
    TIMECAPS xTimeCaps;

    (void)dwCtrlType;

    if (timeGetDevCaps(&xTimeCaps, sizeof(xTimeCaps)) == MMSYSERR_NOERROR)
    {
        /* Match the call to timeBeginPeriod( xTimeCaps.wPeriodMin ) made when
        the process started with a timeEndPeriod() as the process exits. */
        timeEndPeriod(xTimeCaps.wPeriodMin);
    }

    return FALSE;
}
/*-----------------------------------------------------------*/

OS_STK *OSTaskStkInit(void (*task)(void *pd), void *p_arg, OS_STK *ptos, INT16U opt)
{
    static int id = 0;
    ThreadState_t *pxThreadState = NULL;
    int8_t *pcTopOfStack = (int8_t *)ptos;
    const SIZE_T xStackSize = 1024; /* Set the size to a small number which will get rounded up to the minimum possible. */

    /* In this simulated case a stack is not initialised, but instead a thread
    is created that will execute the task being created.  The thread handles
    the context switching itself.  The ThreadState_t object is placed onto
    the stack that was created for the task - so the stack buffer is still
    used, just not in the conventional way.  It will not be used for anything
    other than holding this structure. */
    pxThreadState = (ThreadState_t *)(pcTopOfStack - sizeof(ThreadState_t));

    /* Create the thread itself. */
    pxThreadState->pvThread = CreateThread(NULL, xStackSize, (LPTHREAD_START_ROUTINE)task, p_arg, CREATE_SUSPENDED | STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);

    SetThreadAffinityMask(pxThreadState->pvThread, 0x01);
    SetThreadPriorityBoost(pxThreadState->pvThread, TRUE);
    SetThreadPriority(pxThreadState->pvThread, portTASK_THREAD_PRIORITY);

    return (OS_STK *)pxThreadState;
}
/*-----------------------------------------------------------*/

void OSTCBInitHook(OS_TCB *p_tcb)
{

}

void OSStartHighRdy(void)
{
    void *pvHandle = NULL;
    int32_t lSuccess;
    ThreadState_t *pxThreadState = NULL;
    SYSTEM_INFO xSystemInfo;

    /* This port runs windows threads with extremely high priority.  All the
    threads execute on the same core - to prevent locking up the host only start
    if the host has multiple cores. */
    GetSystemInfo(&xSystemInfo);
    if (xSystemInfo.dwNumberOfProcessors <= 1)
    {
        printf("This version of the FreeRTOS Windows port can only be used on multi-core hosts.\r\n");
        lSuccess = FALSE;
    }
    else
    {
        lSuccess = TRUE;

        /* The highest priority class is used to [try to] prevent other Windows
        activity interfering with FreeRTOS timing too much. */
        if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS) == 0)
        {
            printf("SetPriorityClass() failed\r\n");
        }

        /* Create the events and mutexes that are used to synchronise all the
        threads. */
        pvInterruptEventMutex = CreateMutex(NULL, FALSE, NULL);
        pvInterruptEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if ((pvInterruptEventMutex == NULL) || (pvInterruptEvent == NULL))
        {
            lSuccess = FALSE;
        }

        /* Set the priority of this thread such that it is above the priority of
        the threads that run tasks.  This higher priority is required to ensure
        simulated interrupts take priority over tasks. */
        pvHandle = GetCurrentThread();
        if (pvHandle == NULL)
        {
            lSuccess = FALSE;
        }
    }

    if (lSuccess == TRUE)
    {
        if (SetThreadPriority(pvHandle, portSIMULATED_INTERRUPTS_THREAD_PRIORITY) == 0)
        {
            lSuccess = FALSE;
        }
        SetThreadPriorityBoost(pvHandle, TRUE);
        SetThreadAffinityMask(pvHandle, 0x01);
    }

    if (lSuccess == TRUE)
    {
        /* Start the thread that simulates the timer peripheral to generate
        tick interrupts.  The priority is set below that of the simulated
        interrupt handler so the interrupt event mutex is used for the
        handshake / overrun protection. */
        pvHandle = CreateThread(NULL, 0, thread_timer, NULL, CREATE_SUSPENDED, NULL);
        if (pvHandle != NULL)
        {
            SetThreadPriority(pvHandle, portSIMULATED_TIMER_THREAD_PRIORITY);
            SetThreadPriorityBoost(pvHandle, TRUE);
            SetThreadAffinityMask(pvHandle, 0x01);
            ResumeThread(pvHandle);
        }

        /* Start the highest priority task by obtaining its associated thread
        state structure, in which is stored the thread handle. */
        pxThreadState = (ThreadState_t *)OSTCBHighRdy->OSTCBStkPtr;

        /* Start the first task. */
        ResumeThread(pxThreadState->pvThread);

        /* Handle all simulated interrupts - including yield requests and
        simulated ticks. */
        prvProcessSimulatedInterrupts();
    }

}

/*-----------------------------------------------------------*/

static void prvProcessSimulatedInterrupts(void)
{
    uint32_t ulSwitchRequired, i;
    ThreadState_t *pxThreadState;
    void *pvObjectList[2];
    CONTEXT xContext;

    /* Going to block on the mutex that ensured exclusive access to the simulated
    interrupt objects, and the event that signals that a simulated interrupt
    should be processed. */
    pvObjectList[0] = pvInterruptEventMutex;
    pvObjectList[1] = pvInterruptEvent;

    /* Create a pending tick to ensure the first task is started as soon as
    this thread pends. */
    ulPendingInterrupts = TRUE;
    SetEvent(pvInterruptEvent);

    OSRunning = TRUE;

    for (;;)
    {
        xInsideInterrupt = FALSE;
        WaitForMultipleObjects(sizeof(pvObjectList) / sizeof(void *), pvObjectList, TRUE, INFINITE);

        /* Cannot be in a critical section to get here.  Tasks that exit a
        critical section will block on a yield mutex to wait for an interrupt to
        process if an interrupt was set pending while the task was inside the
        critical section.  xInsideInterrupt prevents interrupts that contain
        critical sections from doing the same. */
        xInsideInterrupt = TRUE;

        /* Used to indicate whether the simulated interrupt processing has
        necessitated a context switch to another task/thread. */
        ulSwitchRequired = FALSE;

        /* Is the simulated interrupt pending? */
        if (ulPendingInterrupts)
        {
            /* Clear the interrupt pending bit. */
            ulPendingInterrupts = 0;
            OSIntEnter();
            OSTimeTick();
            OSIntExit();
        }
        

        ReleaseMutex(pvInterruptEventMutex);
    }
}
/*-----------------------------------------------------------*/

void OSIntCtxSw (void) {
    OSCtxSw();
}

void OSCtxSw(void)
{
    ThreadState_t *p_stk;
    ThreadState_t *p_stk_new;

    OS_CPU_SR    cpu_sr = 0;;

    p_stk = (ThreadState_t *)OSTCBCur->OSTCBStkPtr;

    OSTaskSwHook();

    OSTCBCur = OSTCBHighRdy;
    OSPrioCur = OSPrioHighRdy;

    p_stk_new = (ThreadState_t *)OSTCBHighRdy->OSTCBStkPtr;

    ResumeThread(p_stk_new->pvThread);
    SuspendThread(p_stk->pvThread);

}

static volatile BOOLEAN lMutexNeedsReleasing = FALSE;

void OSDisableInterruptFlag(OS_CPU_SR *pCpu_Sr)
{
    (void) pCpu_Sr;

    if (OSRunning && !lMutexNeedsReleasing)
    {
        /* The interrupt event mutex is held for the entire critical section,
        effectively disabling (simulated) interrupts. */
        WaitForSingleObject(pvInterruptEventMutex, INFINITE);
    }

    lMutexNeedsReleasing = TRUE;

}
/*-----------------------------------------------------------*/

void OSEnableInterruptFlag(OS_CPU_SR *pCpu_Sr)
{
    (void) pCpu_Sr;

    if (pvInterruptEventMutex != NULL)
    {
        if (lMutexNeedsReleasing == TRUE)
        {
            ReleaseMutex(pvInterruptEventMutex);
        }
    }
}
/*-----------------------------------------------------------*/

#if OS_TMR_EN > 0
static  INT16U  OSTmrCtr;
#endif

void OSInitHookBegin(void)
{
#if OS_TMR_EN > 0
    OSTmrCtr = 0;
#endif
}

void OSTimeTickHook(void)
{
#if OS_TMR_EN > 0
    OSTmrCtr++;
    if (OSTmrCtr >= (OS_TICKS_PER_SEC / OS_TMR_CFG_TICKS_PER_SEC)) {
        OSTmrCtr = 0;
        OSTmrSignal();
    }
#endif 
}

void OSTaskIdleHook(void)
{
    Sleep(1u); /* Reduce CPU utilization.                                  */
}

void OSTaskDelHook(OS_TCB *p_tcb)
{

}