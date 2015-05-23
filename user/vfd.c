#include "driver/uart.h"
#include "vfd.h"

/*
 * The command clears display and memory.
 * The cursor shifts to the left end of the upper row.
 */
void vfd_reset() {
   uart_tx_one_char(UART1, 0x0c);
}

/*
 * All characters are cleared while 
 * the cursor remains at the same position.
 */
void vfd_clear() {
   uart_tx_one_char(UART1, 0x0a);
}

/*
 * position is defined by one byte data after the ESC data.
 */
void vfd_pos(uint8_t x, uint8_t y) {
   uart_tx_one_char(UART1, 0x1b);
   uart_tx_one_char(UART1, y*20+x);
}
/*
 * Dimming: 1:100%, 2:75%, 3:50% 4:25%
 */
void vfd_dim(uint8_t brightness) {
   uart_tx_one_char(UART1, 0x4);
   uart_tx_one_char(UART1, brightness);
}

/*
 * Horizontal Scroll Mode
 * All characters are shifted one character to the left and the character
 * written newly is displayed at the right end of the lower row when the
 * writing position reaches the righ t end of the lower row.
 */

void vfd_scroll_mode() {
   uart_tx_one_char(UART1, '\x12');
}

/*
 * Ordinary mode
 * The cursor shifts one character to the right automatically when a
 * character data is written.  If the cursor is at the right end of the upper
 * row, it shifts to the left end of the lower row. If the cursor is at the right
 * end of the lower row, it shifts to the left end of the upper row.
 **/

void vfd_overwrite_mode() {
   uart_tx_one_char(UART1, '\x11');
}

void vfd_print(const char* str) {
    while(*str){
        uart_tx_one_char(UART1, *str);
        str++;
    }
}
