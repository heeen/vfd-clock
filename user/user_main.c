#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "driver/uart.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

unsigned int time = 0;
void some_timerfunc(void *arg) {
    time++;
    uart_tx_one_char(0,'A' + (time % 26));
    uart_tx_one_char(0,'\n');
    uart_tx_one_char(1,'a' + (time % 26));
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events) {
    os_delay_us(10);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init() {
    uart_init(115200, 9600);
    // tx, gpio2

    os_timer_disarm(&some_timer);
    os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

    //0 for once and 1 for repeating
    os_timer_arm(&some_timer, 500, 1);

    system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
}
