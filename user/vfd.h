#ifndef VFD_H
#define VFD_H

void vfd_reset();
void vfd_clear();
void vfd_pos(uint8_t x, uint8_t y);
void vfd_dim(uint8_t brightness);
void vfd_scroll_mode();
void vfd_overwrite_mode();
void vfd_print(const char* string);
#endif /* VFD_H */
