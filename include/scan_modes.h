#ifndef SCAN_MODES_H
#define SCAN_MODES_H

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    SCAN_ECHO,
    SCAN_PASSIVE,
    SCAN_AT
} ScanMode_t;

typedef void (*scan_callback_t)(const char *line, int color_tag);

/* Scan modes return 1 if match found, 0 otherwise.
   *found_baud is set to the successful baud rate. */
int scan_echo_mode(const char *port_name, scan_callback_t log_line, int *found_baud);
int scan_passive_mode(const char *port_name, scan_callback_t log_line, int *found_baud);
int scan_at_mode(const char *port_name, scan_callback_t log_line, int *found_baud);

#endif /* SCAN_MODES_H */
