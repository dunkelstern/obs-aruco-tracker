#ifndef serial_h__included
#define serial_h__included

#include <stdint.h>

extern "C" {

int serial_open(const char *file, int baudrate);
void serial_close(int fd);
void serial_write(int fd, uint64_t timestamp, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

};

#endif /* serial_h__included */