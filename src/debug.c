#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#if configCHECK_FOR_STACK_OVERFLOW

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName )
{
    panic("Stack overflow in task %s", pcTaskName);
}

#endif


#if configGENERATE_RUN_TIME_STATS
void vMainConfigureTimerForRunTimeStats()
{
}

uint64_t ulMainGetRuntimeCounterValue(void)
{
    return to_us_since_boot(get_absolute_time());
}



static int task_cmp(const void *a_, const void *b_) 
{
    const TaskStatus_t *a = (const TaskStatus_t*)a_;
    const TaskStatus_t *b = (const TaskStatus_t*)b_;
    if (a->uxCurrentPriority==b->uxCurrentPriority) {
        return strcmp(a->pcTaskName, b->pcTaskName);
    }
    return a->uxCurrentPriority>b->uxCurrentPriority ? -1 : 1;
}


void print_tasks_info()
{
    TaskStatus_t *status = NULL;
    UBaseType_t status_size;
    UBaseType_t status_count;
    uint64_t total_runtime = 0;

    status_size = uxTaskGetNumberOfTasks() + 5;
    status = calloc(status_size, sizeof(TaskStatus_t));

    status_count = uxTaskGetSystemState(status, status_size, &total_runtime);

    qsort(status, status_count, sizeof(TaskStatus_t), task_cmp);


    printf("Name           State  Pri  Core   Stack               CPU\n");
    printf("---------------------------------------------------------\n");

    total_runtime /= 100;
    for (uint i=0; i<status_count; i++) {
        TaskStatus_t *st = &status[i];
        char state = ' ';
        uint64_t percentage = st->ulRunTimeCounter / total_runtime;

        switch (st->eCurrentState) {
            case eRunning:     /* A task is querying the state of itself, so must be running. */
                state = 'X';
                break;
            case eReady:       /* The task being queried is in a ready or pending ready list. */
                state = 'R';
                break;
            case eBlocked:     /* The task being queried is in the Blocked state. */
                state = 'B';
                break;
            case eSuspended:   /* The task being queried is in the Suspended state, or is in the Blocked state with an infinite time out. */
                state = 'S';
                break;
            case eDeleted:     /* The task being queried has been deleted, but its TCB has not yet been freed. */
                state = 'D';
                break;
            case eInvalid:     /* Used as an 'invalid state' value. */
                state = '!';
                break;
        }

        printf("%-16s %c     %2u     %c %7lu %12lu %3llu%%\n",
            st->pcTaskName, 
            state,
            st->uxCurrentPriority,
            #if ( configUSE_CORE_AFFINITY )
            st->uxCoreAffinityMask==tskNO_AFFINITY ? '-' : ('0'+(st->uxCoreAffinityMask>>1)),
            #else
            '?',
            #endif
            st->usStackHighWaterMark,
            st->ulRunTimeCounter,
            percentage
            );
    }

    free(status);
}

#endif


