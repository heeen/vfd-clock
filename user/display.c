#include <user_interface.h>
#include <osapi.h>
#include <mem.h>

#include "display.h"
#include "ntp.h"
#include "vfd.h"
#include "driver/uart.h"

extern int rssi;

status_line* create_status(const char* text);
void destroy_status(status_line* s);

static status_line* head = 0;
static status_line* tail = 0;
static status_line* current = 0;
static bool statusline_dirty = false;

static os_timer_t display_update_timer;
static int scrollpos = 0;
void update_display(void *arg);
void dumpstatus();
void append(status_line* s);

static char temp[21];
static bool disabled = false;

void statusline(const char* text, unsigned short duration) {
    status_line* s = create_status(text);
    s->until = gettime() + duration;
    append(s);
    dumpstatus();
}

void append(status_line* s) {
    if(tail)
        tail->next = s;
    tail = s;
    if(!head)
        head = s;
    statusline_dirty = true;
}

void dumpstatus() {
    os_printf("head: %p cur: %p tail: %p\n",head, current, tail);
    status_line* s = head;

    int i = 0;
    while(s) {
        os_printf("%d: %p %s\n",i++,s,s->text);
        s = s->next;
    }
}

status_line* create_status(const char* text){
    os_printf("%s\n", text);
    status_line* s = (status_line*) os_zalloc(sizeof(status_line));
    if(!s) {
        os_printf("status alloc failed!");
        return 0;
    }
    os_memset(s, 0, sizeof(status_line));
    int len = os_strlen(text);
    s->text = (char*) os_zalloc(len+1);
    if(!s->text) {
        os_free(s);
        os_printf("status alloc failed!");
        return 0;
    }
    s->len = len;
    os_strcpy(s->text, text);
    os_printf("create status: %p %s %d\n", s, s->text, system_get_free_heap_size());
    return s;
}

void destroy_status(status_line* s) {
    status_line* i = head;
    if(s == tail) tail = 0;

    if(s == head) {
        head = head->next;
    } else {
        // find previous
        while(i && i->next != s) i = i->next;
        i->next = s->next;
    }

    os_free(s->text);
    os_free(s);
    dumpstatus();
}


void start_display() {
  disabled = false;
  os_timer_disarm(&display_update_timer);
  os_timer_setfn(&display_update_timer, (os_timer_func_t *)update_display, NULL);
  os_timer_arm(&display_update_timer, 250, 0);
  statusline("display enabled", 1);
}

void stop_display() {
  disabled = true;
  os_timer_disarm(&display_update_timer);
}

void ICACHE_FLASH_ATTR
update_display_time(time_t t) {
    static time_t last;
    if(t == last) return;
    struct tm *dt = gmtime(&t);
    bool nightMode = false;

    if(dt->tm_hour > 22 || dt->tm_hour < 7) nightMode = true;

    if(nightMode && dt->tm_sec) return;

    if(nightMode)
        os_sprintf(temp, "%02d:%02d", dt->tm_hour, dt->tm_min);
    else
        os_sprintf(temp, "%02d:%02d:%02d", dt->tm_hour, dt->tm_min, dt->tm_sec);

    vfd_pos(0,0);
    vfd_print(temp);
    last = t;
}

bool ICACHE_FLASH_ATTR
update_display_statusline(time_t timestamp) {
    bool scrolling = false;
    if(!statusline_dirty)
        return scrolling;

    if(!current && head)
        current = head;

    vfd_pos(0,1);
    if(current) {
        int x = 0;
        for(x = 0; x < 20; x++) {
            int i = x + scrollpos;
            if(i == -2 || i == current->len + 1)
                uart_tx_one_char(UART1, '*');
            else if(i < 0 || i >= current->len)
                uart_tx_one_char(UART1, ' ');
            else
                uart_tx_one_char(UART1, current->text[i]);
        }
        scrollpos ++;
        if((current->len <= 20 && scrollpos > 3)
          || (current->len > 20 && scrollpos > current->len - 20 + 3)) {
            scrollpos = -3;
            status_line* c = current;
            current = current->next;
            statusline_dirty = true;
            if(c->until < timestamp) {
                destroy_status(c);
            }
        } else {
            scrolling = true;
        }
    } else {
      if(statusline_dirty) {
        vfd_print("                    ");
        statusline_dirty = false;
      }
    }
    return scrolling;

}

void ICACHE_FLASH_ATTR
update_display(void *arg)
{
    if(disabled) return;
    time_t timestamp = gettime();
    bool scrolling = false;

    update_display_time(timestamp);

    vfd_pos(18, 0);
    if(timestamp - last_ntp_update > 30*60) {
      statusline("fetching network time", 1);
      ntp_get_time();
      vfd_print("?");
    } else {
      vfd_print(" ");
    }

    vfd_bars_char(128, rssi);
    uart_tx_one_char(UART1, 128);

    scrolling = update_display_statusline(timestamp);

    os_timer_arm(&display_update_timer, scrolling ? 250 : 1000, 0);
}

void display_small_update() {
}
