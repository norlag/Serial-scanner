#define _POSIX_C_SOURCE 200809L
#include "scan_modes.h"
#include "libserialport.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Echo scan mode - returns 1 if echo detected, 0 otherwise */
int scan_echo_mode(const char *port_name, scan_callback_t log_line, int *found_baud) {
    if (!port_name || !log_line) return 0;
    log_line("[INFO] Testing echo loopback", 0);

    struct sp_port *port = NULL;
    if (sp_get_port_by_name(port_name, &port) != SP_OK) {
        log_line("[ERROR] Cannot open port", 4);
        return 0;
    }

    if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
        log_line("[ERROR] Failed to open port", 4);
        sp_free_port(port);
        return 0;
    }

    /* Scan through standard baudrates */
    int baud_rates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    int num_bauds = 8;

    for (int i = 0; i < num_bauds; i++) {
        sp_set_baudrate(port, baud_rates[i]);
        sp_set_bits(port, 8);
        sp_set_parity(port, SP_PARITY_NONE);
        sp_set_stopbits(port, 1);

        /* Flush buffers */
        sp_flush(port, SP_BUF_INPUT | SP_BUF_OUTPUT);

        char msg[128];
        snprintf(msg, sizeof(msg), "[TRY] Baud %d", baud_rates[i]);
        log_line(msg, 1);

        /* Send test character using sp_blocking_write (timeout in ms) */
        char test_char = 'A';
        enum sp_return write_ret = sp_blocking_write(port, &test_char, 1, 200);
        if (write_ret != SP_OK) {
            continue;
        }

        /* Wait for echo with 200ms timeout (timeout is in ms) */
        char buffer[16];
        ssize_t count = sp_blocking_read(port, buffer, sizeof(buffer), 200);

        if (count > 0) {
            /* Check if we got the echo character */
            int echo_found = 0;
            for (ssize_t j = 0; j < count; j++) {
                if (buffer[j] == test_char || (buffer[j] >= 0x20 && buffer[j] <= 0x7E)) {
                    echo_found = 1;
                    break;
                }
            }

            if (echo_found) {
                snprintf(msg, sizeof(msg), "[SUCCESS] Echo detected at %d baud", baud_rates[i]);
                log_line(msg, 3);
                *found_baud = baud_rates[i];
                sp_close(port);
                sp_free_port(port);
                return 1;
            }
        }
    }

    log_line("[TIMEOUT] No echo detected", 2);
    sp_close(port);
    sp_free_port(port);
    return 0;
}
