#include "user_interface.h"
#include "mem.h"
#include <osapi.h>
#include "wificfg.h"
#include "display.h"

static void  ICACHE_FLASH_ATTR
scan_done(void *arg, STATUS status)
{
  if (status != OK) {
    statusline("wifi scan errorn", 1);
    return;
  }

  struct bss_info *bss_link = (struct bss_info *)arg;
  bss_link = bss_link->next.stqe_next; // ignore first

  int i=0;
  while (bss_link != NULL) {
    int j=0;
    for(j=0; wifis[j][0] != 0; j++) {
      const char* ssid = wifis[j][0];
      const char* pass = wifis[j][1];
      if(strcmp(ssid, bss_link->ssid) == 0) {
        char temp[128];
        os_sprintf(temp, "found known network %s", ssid);
        statusline(temp, 2);
        struct station_config stationConf;
        os_bzero(&stationConf, sizeof(struct station_config));
        os_strcpy(&stationConf.ssid, ssid);
        os_strcpy(&stationConf.password, pass);
        wifi_station_disconnect();
        ETS_UART_INTR_DISABLE();
        wifi_station_set_config(&stationConf);
        ETS_UART_INTR_ENABLE();
        wifi_station_connect();
        break;
      }
    }
    bss_link = bss_link->next.stqe_next;
    i++;
  }
  os_printf("XXX scanning results end\n");
}

void ICACHE_FLASH_ATTR
connect_known_ap() {
    ETS_UART_INTR_DISABLE();
    wifi_set_opmode(STATION_MODE);
    ETS_UART_INTR_ENABLE();

    statusline("starting WIFI scan", 1);
    if(!wifi_station_scan(NULL, scan_done)) {
        statusline("WIFI scan failed!", 1);
    }
}
