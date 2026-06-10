#define _POSIX_C_SOURCE 200809L
#include "scan_modes.h"
#include "libserialport.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Passive scan mode - returns 1 if valid ASCII stream found */
int scan_passive_mode(const char *port_name, scan_callback_t log_line, int *found_baud) {
    if (!port_name || !log_line) return 0;
    log_line("[INFO] Passive listen for data stream", 0);

    struct sp_port *port = NULL;
    if (sp_get_port_by_name(port_name, &port) != SP_OK) {
        log_line("[ERROR] Cannot open port", 4);
        return 0;
    }

    if (sp_open(port, SP_MODE_READ) != SP_OK) {
        log_line("[ERROR] Failed to open port", 4);
        sp_free_port(port);
        return 0;
    }

    int baud_rates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    int num_bauds = 8;

    for (int i = 0; i < num_bauds; i++) {
        sp_set_baudrate(port, baud_rates[i]);
        sp_set_bits(port, 8);
        sp_set_parity(port, SP_PARITY_NONE);
        sp_set_stopbits(port, 1);

        char msg[128];
        snprintf(msg, sizeof(msg), "[TRY] Passive listen at %d baud", baud_rates[i]);
        log_line(msg, 1);

        /* Listen for up to 2 seconds, polling every 100ms */
        char buffer[512];
        int valid_chars = 0;
        int total_bytes = 0;

        for (int ms = 0; ms < 2000 && total_bytes < (int)sizeof(buffer); ms += 100) {
            /* sp_blocking_read timeout is in ms */
            ssize_t count = sp_blocking_read(port, buffer + total_bytes, sizeof(buffer) - total_bytes, 100);
            if (count > 0) {
                total_bytes += count;
                for (int j = 0; j < count; j++) {
                    unsigned char c = buffer[total_bytes - count + j];
                    if ((c >= 0x20 && c <= 0x7E) || c == '\n' || c == '\r' || c == '\t') {
                        valid_chars++;
                    }
                }
            }
        }

        if (valid_chars > 5) {
            snprintf(msg, sizeof(msg), "[SUCCESS] Valid ASCII stream at %d baud (%d valid chars)", baud_rates[i], valid_chars);
            log_line(msg, 3);
            *found_baud = baud_rates[i];
            sp_close(port);
            sp_free_port(port);
            return 1;
        }
    }

    log_line("[TIMEOUT] No valid data stream detected", 2);
    sp_close(port);
    sp_free_port(port);
    return 0;
}
