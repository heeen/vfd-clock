#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "driver/uart.h"
#include "user_interface.h"

#include "ntp.h"

#include "espconn.h"
#include "mem.h"
#include "wifimgr.h"

#include "vfd.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

struct mdns_info *s_mdns_info=0;
struct ip_info s_ip;
char temp[128];
os_timer_t wifi_check_timer;
os_timer_t display_time_timer;

void joinAP();
void checkmDns();
void displayTime();
void check_ap_joined(void *arg);

void ICACHE_FLASH_ATTR
print(const char *str) {
    while(*str){
        if(*str < 31)
            uart_tx_one_char(UART1, ' ');
        else
            uart_tx_one_char(UART1, *str);
        uart_tx_one_char(UART0, *str);
        str++;
    }
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events) {
    os_delay_us(10);
}

void ICACHE_FLASH_ATTR
user_init() {
  uart_init(115200, 9600); // tx, gpio2
  vfd_reset();
  vfd_dim(3);
  vfd_overwrite_mode();

  system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

  char macaddr[6];
  wifi_get_macaddr(STATION_IF, macaddr);
  os_sprintf(temp, "<"MACSTR">", MAC2STR(macaddr));
  print(temp);
  
  os_timer_disarm(&wifi_check_timer);
  os_timer_setfn(&wifi_check_timer, (os_timer_func_t *)check_ap_joined, NULL);
  os_timer_arm(&wifi_check_timer, 1000, 1);
}



void ICACHE_FLASH_ATTR
check_ap_joined(void *arg)
{
  static uint32_t checkTime = 0;
  uint8_t status;
  static uint8_t prev_status = STATION_IDLE;

  checkTime++;
  status = wifi_station_get_connect_status();
  if(prev_status != status) {
      prev_status = status;
      switch(status) {
        case STATION_IDLE:
          print("\fidle");
          break;
        case STATION_CONNECTING:
          print("\fconnecting");
          break;
        case STATION_WRONG_PASSWORD:
          print("\fwrong password");
          break;
        case STATION_NO_AP_FOUND:
          print("\fno ap found");
          connect_known_ap();
          break;
        case STATION_CONNECT_FAIL:
          print("\fconnect failed");
          break;
        case STATION_GOT_IP:
          print("\fconnected!");
          //checkmDns();
          vfd_clear();
          ntp_get_time();

/*          os_timer_disarm(&display_time_timer);
          os_timer_setfn(&display_time_timer, (os_timer_func_t *)displayTime, NULL);
          os_timer_arm(&display_time_timer, 1000, 1);*/
          break;
      }
  }
  if(timestamp) {
    displayTime();
  }
}


void ICACHE_FLASH_ATTR
checkmDns() {
  if(!s_mdns_info) {
    s_mdns_info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));
    s_mdns_info->host_name = "vfd-clock";
    s_mdns_info->ipAddr = s_ip.ip.addr;
    s_mdns_info->server_name = "iot";
    s_mdns_info->server_port = 8080;
    s_mdns_info->txt_data[0] = "foo = bar";
    espconn_mdns_init(s_mdns_info);
    //espconn_mdns_server_register();
    espconn_mdns_enable();
  }
}


void ICACHE_FLASH_ATTR
displayTime() {
    struct tm *dt = gmtime(&timestamp);
    char timestr[16];
    os_sprintf(timestr, "%02d:%02d:%02d", dt->tm_hour, dt->tm_min, dt->tm_sec);
    vfd_pos(0,0);
    vfd_print(timestr);

    if(timestamp - last_ntp_update > 30*60) {
      print("ntp update.");
      ntp_get_time();
      vfd_pos(19, 0);
      vfd_print("?");
    } else {
      vfd_bars_char(128, wifi_station_get_rssi());
      vfd_pos(19, 0);
      uart_tx_one_char(UART1, 128);
    }

    wifi_get_ip_info(0x00, &s_ip);
    os_sprintf(temp, "%d.%d.%d.%d", IP2STR(&s_ip.ip));
    int p = 20 - os_strlen(temp) - 1;
    vfd_pos(p,1);
    vfd_print(temp);
}

