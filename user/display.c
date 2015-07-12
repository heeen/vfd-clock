#include <user_interface.h>
#include <osapi.h>
#include <mem.h>

#include "display.h"
#include "ntp.h"
#include "vfd.h"
#include "driver/uart.h"

status_line* create_status(const char* text);
void destroy_status(status_line* s);

static status_line* head = 0;
static status_line* current = 0;

static os_timer_t display_update_timer;
static int scrollpos = -20;
void update_display(void *arg);

void showstatus(const char* text, unsigned short duration) {
    status_line* s = create_status(text);
    s->next = head;
    head = s;
}

status_line* create_status(const char* text){
    status_line* s = (status_line*) os_zalloc(sizeof(status_line));
    os_memset(s, 0, sizeof(status_line));
    s->text = (char*) os_zalloc(os_strlen(text));
    os_strcpy(s->text, text);
    return s;
}

void destroy_status(status_line* s) {
    status_line* i = head;
    if(s == head) {
        head = 0;
    } else if(s->next) {
        // find previous
        while(i && i->next != s) i = i->next;
        i->next = s->next;
    }

    os_free(s->text);
    os_free(s);
}


void start_display() {
  os_timer_disarm(&display_update_timer);
  os_timer_setfn(&display_update_timer, (os_timer_func_t *)update_display, NULL);
  os_timer_arm(&display_update_timer, 200, 1);
  showstatus("display enabled", 3);
}

void stop_display() {
  os_timer_disarm(&display_update_timer);
}

void ICACHE_FLASH_ATTR
update_display(void *arg)
{
    time_t timestamp = gettime();
    struct tm *dt = gmtime(&timestamp);
    bool nightMode = false;
    if(dt->tm_hour > 22 || dt->tm_hour < 7) nightMode = true;
    char timestr[21];
    if(nightMode)
        os_sprintf(timestr, "%02d:%02d              ", dt->tm_hour, dt->tm_min, dt->tm_sec);
    else
        os_sprintf(timestr, "%02d:%02d:%02d           ", dt->tm_hour, dt->tm_min, dt->tm_sec);
    vfd_pos(0,0);
    vfd_print(timestr);
  

    if(timestamp - last_ntp_update > 30*60) {
      ntp_get_time();
      vfd_pos(18, 0);
      vfd_print("?");
    } 
    if(timestamp) {
        vfd_bars_char(128, wifi_station_get_rssi());
        vfd_pos(19, 0);
        uart_tx_one_char(UART1, 128);
    }

    if(!current && head)
        current = head;

    vfd_pos(0,1);
    if(current) {
        scrollpos ++;
        if(scrollpos > current->len) scrollpos = -20;
        int x = 0;
        for(x = 0; x < 20; x++) {
            int i = x + scrollpos;
            if(i < 0 || i > current->len)
                uart_tx_one_char(UART1, ' ');
            else
                uart_tx_one_char(UART1, current->text[i]);
        }
    } else {
      vfd_print("----");
    }
}
