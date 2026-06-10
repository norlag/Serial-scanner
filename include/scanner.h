#ifndef SCANNER_H
#define SCANNER_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include "device.h"
#include "scan_modes.h"

/* Scanner state machine */
typedef enum {
    SCANNER_STATE_IDLE,
    SCANNER_STATE_SCANNING,
    SCANNER_STATE_COMPLETE
} ScannerState_t;

/* Full scanner structure - defined in header for opaque pointer usage */
typedef struct Scanner {
    char *port_name;
    ScanMode_t mode;
    pthread_t thread;
    int running;
    scan_callback_t log_line;
    void (*on_success)(const char *port, int baud);
    ScannerState_t state;
    int found_baud;
} Scanner_t;

Scanner_t *scanner_create(const char *port_name, ScanMode_t mode);
void scanner_destroy(Scanner_t **scanner);
int scanner_start(Scanner_t *scanner);
int scanner_stop(Scanner_t *scanner);
void scanner_set_mode(Scanner_t *scanner, ScanMode_t mode);
void scanner_set_ui_callbacks(Scanner_t *scanner, scan_callback_t log_line, void (*on_success)(const char *port, int baud));

#endif /* SCANNER_H */
