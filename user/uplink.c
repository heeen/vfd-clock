#include "uplink.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "os_type.h"
#include "user_interface.h"


#include "c_types.h"
#include "user_interface.h"
#include "mem.h"
#include "osapi.h"
#include "upgrade.h"
#include "espconn.h" 
#include "rboot.h"
#include "rboot-ota.h"
#include "ntp.h"

static struct espconn uplink_conn;
static esp_tcp uplink_tcp_conn;
static void uplink_connectedCb(void *arg);
static void uplink_disconCb(void *arg);
static void uplink_reconCb(void *arg, sint8 err);
static void uplink_recvCb(void *arg, char *data, unsigned short len);
static void uplink_sentCb(void *arg);
const char* esp_errstr(sint8 err);
static os_timer_t recon_timer;
static os_timer_t alive_timer;

static ip_addr_t mothership_ip;
const char* const mothership_hostname = "heeen.de";
void ICACHE_FLASH_ATTR mothership_resolved(const char *name, ip_addr_t *ipaddr, void *arg);

void uplink_init();
void uplink_ota();
static void ICACHE_FLASH_ATTR tcp_print(struct espconn* con, char* str);

void uplink_start() {
  print("connecting to the mothership\n");
  print("looking up <");
  print(mothership_hostname);
  print(">\n");
  espconn_gethostbyname(&uplink_conn, mothership_hostname, &mothership_ip,
            mothership_resolved);
}

void uplink_stop() {
  os_timer_disarm(&recon_timer);
}

void ICACHE_FLASH_ATTR
do_alive(void *arg) {
  tcp_print(&uplink_conn, "alive\n");
}

void ICACHE_FLASH_ATTR
do_reconnect(void *arg) {
    uplink_start();
}

void uplink_init() {

}

void ICACHE_FLASH_ATTR
mothership_resolved(const char *name, ip_addr_t *ipaddr, void *arg)
{
  if(!ipaddr) {
      print("could not resolve!\n");
      uplink_start();
  }
  os_printf("resolved! %p\n", ipaddr);
  struct espconn *pespconn = (struct espconn *)arg;
  os_printf("mothership resolved to %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

  uplink_conn.type = ESPCONN_TCP;
  uplink_conn.state = ESPCONN_NONE;
  uplink_conn.proto.tcp = &uplink_tcp_conn;
  uplink_conn.proto.tcp->local_port = espconn_port();
  uplink_conn.proto.tcp->remote_port = 7778;
  os_memcpy(uplink_conn.proto.tcp->remote_ip, &ipaddr->addr, 4);
  os_memcpy(&mothership_ip.addr, &ipaddr->addr, 4);

  espconn_regist_connectcb(&uplink_conn, uplink_connectedCb);
  espconn_regist_disconcb(&uplink_conn, uplink_disconCb);
  espconn_regist_reconcb(&uplink_conn, uplink_reconCb);
  espconn_regist_recvcb(&uplink_conn, uplink_recvCb);
  espconn_regist_sentcb(&uplink_conn, uplink_sentCb);

  os_timer_disarm(&recon_timer);
  os_timer_setfn(&recon_timer, (os_timer_func_t *) do_reconnect, NULL);

  uint32 nKeepaliveParam = 10;
  espconn_set_keepalive(pespconn, ESPCONN_KEEPIDLE, &nKeepaliveParam);
  nKeepaliveParam = 2;
  espconn_set_keepalive(pespconn, ESPCONN_KEEPINTVL, &nKeepaliveParam);
  nKeepaliveParam = 10;
  espconn_set_keepalive(pespconn, ESPCONN_KEEPCNT, &nKeepaliveParam);
  espconn_set_opt(pespconn,ESPCONN_KEEPALIVE);

  print("connecting!\n");
  espconn_connect(&uplink_conn);
}

static void ICACHE_FLASH_ATTR uplink_sentCb(void *arg) {
  print("sent\n");
}

static void ICACHE_FLASH_ATTR tcp_print(struct espconn* con, char* str) {
  espconn_sent(con, str, strlen(str));
}

static void ICACHE_FLASH_ATTR uplink_recvCb(void *arg, char *data, unsigned short len) {
  char temp[32];
  print("received:");
  struct espconn *conn = (struct espconn *) arg;
  uart0_tx_buffer(data,len);
  print("\n");
  if(strncmp(data, "OTA", 3) == 0) {
      print("got OTA request\n");
      uplink_ota();
  } else if(strncmp(data, "ROM0", 4) == 0) {
      print("load rom0 request\n");
      rboot_set_current_rom(0);
      print("Restarting into rom 0...\n");
      tcp_print(conn, "Restarting into rom 0...\n");
      system_restart();
  } else if(strncmp(data, "ROM1", 4) == 0) {
      print("load rom1 request\n");
      rboot_set_current_rom(1);
      print("Restarting into rom 1...\n");
      tcp_print(conn, "Restarting into rom 1...\n");
      system_restart();
  } else if(strncmp(data, "ping", 4) == 0) {
      tcp_print(conn, "pong\n");
  } else if(strncmp(data, "time", 4) == 0) {
    struct tm *dt = gmtime(&timestamp);
    os_sprintf(temp, "%02d:%02d:%02d", dt->tm_hour, dt->tm_min, dt->tm_sec);
    tcp_print(conn, temp);
  } else if(strncmp(data, "rssi", 4) == 0) {
    os_sprintf(temp, "RSSI=%d\n", wifi_station_get_rssi());
    tcp_print(conn, temp);
  }
}

static void ICACHE_FLASH_ATTR uplink_connectedCb(void *arg) {
  char temp[128];
  sint8 d;
  print("uplink connected\n");
  struct espconn *conn=(struct espconn *)arg;
  char macaddr[6];
  wifi_get_macaddr(STATION_IF, macaddr);
  os_sprintf(temp, 
          "MAC="MACSTR"\n"
          "ROM=%d\n",
          MAC2STR(macaddr),
          rboot_get_current_rom());
  d = espconn_sent(conn, temp, strlen(temp));
  os_sprintf(temp, "HELLO.\n");
  d = espconn_sent(conn, temp, strlen(temp));
  print("-- cend\n");
  os_timer_disarm(&alive_timer);
  os_timer_setfn(&alive_timer, (os_timer_func_t *) do_alive, NULL);
  os_timer_arm(&alive_timer, 15000, 0);
}

static void ICACHE_FLASH_ATTR uplink_reconCb(void *arg, sint8 err) {
  print(esp_errstr(err));
  print("\n");
  os_timer_disarm(&recon_timer);
  os_timer_disarm(&alive_timer);
  os_timer_arm(&recon_timer, 1000, 0);
}

static void ICACHE_FLASH_ATTR uplink_disconCb(void *arg) {
  print("dcon\n");
  os_timer_disarm(&recon_timer);
  os_timer_disarm(&alive_timer);
  os_timer_arm(&recon_timer, 1000, 0);
}



const char* esp_errstr(sint8 err) {
    switch(err) {
        case ESPCONN_OK:
            return "No error, everything OK.";
        case ESPCONN_MEM:
            return "Out of memory error.";
        case ESPCONN_TIMEOUT:
            return "Timeout.";
        case ESPCONN_RTE:
            return "Routing problem.";
        case ESPCONN_INPROGRESS:
            return    "Operation in progress";
        case ESPCONN_ABRT:
            return    "Connection aborted.";
        case ESPCONN_RST:
            return    "Connection reset.";
        case ESPCONN_CLSD:
            return   "Connection closed.";
        case ESPCONN_CONN:
            return   "Not connected.";
        case ESPCONN_ARG:
            return   "Illegal argument.";
        case ESPCONN_ISCONN:
            return   "Already connected.";
    }
}

const char* const http_header =
"GET /esp8266/%s HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Cache-Control: no-cache\r\n"
"User-Agent: rBoot-Sample/1.0\r\n"
"Accept: */*\r\n\r\n";

static void ICACHE_FLASH_ATTR OtaUpdate_CallBack(void *arg, bool result) {

    char msg[40];
    rboot_ota *ota = (rboot_ota*)arg;

    if(result == true) {
        // success, reboot
        os_sprintf(msg, "Firmware updated, rebooting to rom %d...\r\n", ota->rom_slot);
        print(msg);
        rboot_set_current_rom(ota->rom_slot);
        system_restart();
    } else {
        // fail, cleanup
        print("Firmware update failed!\r\n");
        os_free(ota->request);
        os_free(ota);
    }
}

void ICACHE_FLASH_ATTR uplink_ota() {
    uint8 slot;
    rboot_ota *ota;

    // create the update structure
    ota = (rboot_ota*) os_zalloc(sizeof(rboot_ota));
    os_memcpy(ota->ip, &mothership_ip.addr, 4);
    ota->port = 80;
    ota->callback = (ota_callback)OtaUpdate_CallBack;
    ota->request = (uint8 *)os_zalloc(512);

    // select rom slot to flash
    slot = rboot_get_current_rom();
    print("fooo\n");
        
    if (slot == 0)
        slot = 1;
    else
        slot = 0;
    ota->rom_slot = slot;

    // actual http request
    os_sprintf((char*)ota->request,
            http_header,
            (slot == 0 ? "rom0.bin" : "rom1.bin"),
            mothership_hostname);
    print(ota->request);
    // start the upgrade process
    if (rboot_ota_start(ota)) {
        print("Updating...\n");
    } else {
        print("Updating failed!\n");
        os_free(ota->request);
        os_free(ota);
    }
}
