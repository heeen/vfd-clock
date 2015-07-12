#ifndef COMMON_H
#define COMMON_H 
void ICACHE_FLASH_ATTR disable_clock();
void ICACHE_FLASH_ATTR enable_clock();
const char* ICACHE_FLASH_ATTR esp_errstr(sint8 err);
#endif /* COMMON_H */
