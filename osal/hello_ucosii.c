#include <stdio.h>
#include <ucos_ii.h>


#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];

#define TASK1_PRIORITY      1
#define TASK2_PRIORITY      2

static OS_EVENT *pevent;

/* Prints "Hello World" and sleeps for three seconds */
void task1(void* pdata)
{
  while (1)
  { 
    printf("Hello from task1\n");
    OSTimeDlyHMSM(0, 0, 1, 0);
    // getchar();
    OSSemPost(pevent);
  }
}
/* Prints "Hello World" and sleeps for three seconds */
void task2(void* pdata)
{
  INT8U err;
  while (1)
  { 
    // OSTimeDlyHMSM(0, 0, 2, 0);
    OSSemPend(pevent, 100, &err);
    if(err != OS_TIMEOUT)
    {
    	printf("Hello from task2\n");
    }
  }
}
/* The main function creates two task and starts multi-tasking */
int main(void)
{
  OSInit();
  printf("MicroC/OS-II\n");
  pevent = OSSemCreate(1);

  OSTaskCreateExt(task1,
                  NULL,
                  (void *)&task1_stk[TASK_STACKSIZE-1],
                  TASK1_PRIORITY,
                  TASK1_PRIORITY,
                  task1_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);
              
               
  OSTaskCreateExt(task2,
                  NULL,
                  (void *)&task2_stk[TASK_STACKSIZE-1],
                  TASK2_PRIORITY,
                  TASK2_PRIORITY,
                  task2_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);
  OSStart();
  return 0;
}

