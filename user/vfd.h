#ifndef VFD_H
#define VFD_H

void vfd_reset();
void vfd_clear();
void vfd_pos(uint8_t x, uint8_t y);
void vfd_dim(uint8_t brightness);
void vfd_scroll_mode();
void vfd_overwrite_mode();
void vfd_print(const char* string);
void vfd_custom_char(uint8_t code,
        uint8_t row1,
        uint8_t row2,
        uint8_t row3,
        uint8_t row4,
        uint8_t row5,
        uint8_t row6,
        uint8_t row7);
void vfd_bars_char(uint8_t code, int dbm);
#endif /* VFD_H */
