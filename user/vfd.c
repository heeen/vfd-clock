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

void vfd_custom_char(uint8_t code,
        uint8_t row1,
        uint8_t row2,
        uint8_t row3,
        uint8_t row4,
        uint8_t row5,
        uint8_t row6,
        uint8_t row7) {
   uart_tx_one_char(UART1, '\x1a');
   uart_tx_one_char(UART1, code);
   uart_tx_one_char(UART1, row1);
   uart_tx_one_char(UART1, row2);
   uart_tx_one_char(UART1, row3);
   uart_tx_one_char(UART1, row4);
   uart_tx_one_char(UART1, row5);
   uart_tx_one_char(UART1, row6);
   uart_tx_one_char(UART1, row7);
}

/*
 *
 *  below -87 zero (still connected, but barely)
    -87 to -82 1
    -82 to -77 2
    -77 to -72 3
    -72 to -67 4
    above -67 5
 * */

void vfd_bars_char(uint8_t code, int dbm) {
    if(dbm < -106) {
        vfd_custom_char(code,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b10000);
    } else if(dbm < -100) {
        vfd_custom_char(code,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b11100);
    } else if (dbm < -87) {
        vfd_custom_char(code,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b11111);
    } else if (dbm < -82) {
        vfd_custom_char(code,
                0b00000,
                0b00000,
                0b00000,
                0b00000,
                0b00001,
                0b00000,
                0b11111);
    } else if (dbm < -77) {
        vfd_custom_char(code,
                0b00000,
                0b00000,
                0b00000,
                0b00010,
                0b00011,
                0b00000,
                0b11111);
    } else if (dbm < -72) {
        vfd_custom_char(code,
                0b00000,
                0b00000,
                0b00100,
                0b00110,
                0b00111,
                0b00000,
                0b11111);
    } else if (dbm < -67) {
        vfd_custom_char(code,
                0b00000,
                0b01000,
                0b01100,
                0b01110,
                0b01111,
                0b00000,
                0b11111);
    } else {
        vfd_custom_char(code,
                0b10000,
                0b11000,
                0b11100,
                0b11110,
                0b11111,
                0b00000,
                0b11111);
    }
}

void vfd_print(const char* str) {
    while(*str){
        uart_tx_one_char(UART1, *str);
        str++;
    }
}
