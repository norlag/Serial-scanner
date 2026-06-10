#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <stdlib.h>

struct sp_port;

typedef struct {
    char *name;
    char *manufacturer;
    char *description;
    char *path;
    void *port;
} DeviceEntry_t;

DeviceEntry_t *device_list_enumerate(size_t *count);
void device_list_free(DeviceEntry_t **list, size_t count);
int device_open(const char *path, int baud, DeviceEntry_t **device);
int device_close(DeviceEntry_t *device);

#endif // DEVICE_H