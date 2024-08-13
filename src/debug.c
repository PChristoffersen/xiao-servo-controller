#include <stdio.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>

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
#endif


