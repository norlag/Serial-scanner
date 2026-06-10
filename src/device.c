#define _POSIX_C_SOURCE 200809L
#include "device.h"
#include "libserialport.h"
#include <stdlib.h>
#include <string.h>

// Internal helper to copy string safely
static char *safe_strdup(const char *str) {
    if (!str) return strdup("(unknown)");
    char *copy = strdup(str);
    return copy ? copy : strdup("(unknown)");
}

DeviceEntry_t *device_list_enumerate(size_t *count) {
    struct sp_port **ports = NULL;
    size_t num_ports = 0;

    enum sp_return ret = sp_list_ports(&ports);
    if (ret != SP_OK) return NULL;

    // Count ports
    while (ports[num_ports] != NULL) num_ports++;

    if (num_ports == 0) {
        sp_free_port_list(ports);
        return NULL;
    }

    DeviceEntry_t *list = calloc(num_ports, sizeof(DeviceEntry_t));
    if (!list) {
        sp_free_port_list(ports);
        return NULL;
    }

    for (size_t i = 0; i < num_ports; i++) {
        struct sp_port *port = ports[i];
        char *port_name = sp_get_port_name(port);
        char *desc = sp_get_port_description(port);
        char *manufacturer = sp_get_port_usb_manufacturer(port);

        // Build a path from the port name
        list[i].name = safe_strdup(port_name);
        list[i].manufacturer = safe_strdup(manufacturer ? manufacturer : "N/A");
        list[i].description = safe_strdup(desc ? desc : "Serial Port");
        list[i].path = safe_strdup(port_name);
        list[i].port = NULL; // Not opened yet

        free(port_name);
        free(desc);
        free(manufacturer);
    }

    sp_free_port_list(ports);
    *count = num_ports;
    return list;
}

void device_list_free(DeviceEntry_t **list, size_t count) {
    if (!list || !*list) return;

    for (size_t i = 0; i < count; i++) {
        free((*list)[i].name);
        free((*list)[i].manufacturer);
        free((*list)[i].description);
        free((*list)[i].path);
    }
    free(*list);
    *list = NULL;
}

// Open a serial port with the given baud rate
int device_open(const char *path, int baud, DeviceEntry_t **device) {
    if (!path || !device) return -1;

    struct sp_port *port = NULL;
    enum sp_return ret = sp_get_port_by_name(path, &port);
    if (ret != SP_OK) {
        sp_free_port(port);
        return -1;
    }

    // Open the port
    ret = sp_open(port, SP_MODE_READ_WRITE);
    if (ret != SP_OK) {
        sp_free_port(port);
        return -1;
    }

    // Set baud rate
    ret = sp_set_baudrate(port, baud);
    if (ret != SP_OK) {
        sp_close(port);
        sp_free_port(port);
        return -1;
    }

    // Configure: 8N1
    ret = sp_set_bits(port, 8);
    if (ret != SP_OK) {
        sp_close(port);
        sp_free_port(port);
        return -1;
    }
    ret = sp_set_parity(port, SP_PARITY_NONE);
    if (ret != SP_OK) {
        sp_close(port);
        sp_free_port(port);
        return -1;
    }
    ret = sp_set_stopbits(port, 1);
    if (ret != SP_OK) {
        sp_close(port);
        sp_free_port(port);
        return -1;
    }

    // Allocate DeviceEntry_t
    *device = calloc(1, sizeof(DeviceEntry_t));
    if (!*device) {
        sp_close(port);
        sp_free_port(port);
        return -1;
    }
    (*device)->port = port;
    (*device)->path = strdup(path);
    (*device)->name = strdup(path);
    (*device)->manufacturer = strdup("N/A");
    (*device)->description = strdup("Open Port");
    return 0;
}

// Close a serial port
int device_close(DeviceEntry_t *device) {
    if (!device) return -1;

    if (device->port) {
        sp_close(device->port);
        /* sp_open() does not create a new port object, so we don't free it here */
        device->port = NULL;
    }
    free(device->path);
    free(device->name);
    free(device->manufacturer);
    free(device->description);
    free(device);
    return 0;
}
