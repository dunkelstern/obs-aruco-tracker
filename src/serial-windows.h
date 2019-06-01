#ifndef serial_h__included
#define serial_h__included

#include <stdint.h>
#include <obs.h>
#include <obs-module.h>
#include <obs-source.h>

#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>

extern "C" {

typedef HANDLE serial_handle_t;

serial_handle_t serial_open(const char *file, int baudrate);
void serial_close(serial_handle_t fd);
void serial_write(serial_handle_t fd, uint64_t timestamp, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void serial_enumerate(obs_property_t *list_to_update);

};

#endif /* serial_h__included */
