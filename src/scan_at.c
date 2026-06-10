#define _POSIX_C_SOURCE 200809L
#include "scan_modes.h"
#include "libserialport.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* AT command scan mode - returns 1 if OK response received */
int scan_at_mode(const char *port_name, scan_callback_t log_line, int *found_baud) {
    if (!port_name || !log_line) return 0;
    log_line("[INFO] Sending AT command", 0);

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

    int baud_rates[] = {9600, 19200, 4800, 2400, 115200, 57600, 38400, 1200};
    int num_bauds = 8;

    for (int i = 0; i < num_bauds; i++) {
        sp_set_baudrate(port, baud_rates[i]);
        sp_set_bits(port, 8);
        sp_set_parity(port, SP_PARITY_NONE);
        sp_set_stopbits(port, 1);
        sp_flush(port, SP_BUF_INPUT | SP_BUF_OUTPUT);

        char msg[128];
        snprintf(msg, sizeof(msg), "[TRY] AT command at %d baud", baud_rates[i]);
        log_line(msg, 1);

        /* Send AT\r\n using sp_blocking_write (timeout in ms) */
        const char *at_cmd = "AT\r\n";
        enum sp_return write_ret = sp_blocking_write(port, at_cmd, strlen(at_cmd), 500);
        if (write_ret != SP_OK) {
            continue;
        }

        /* Read response with 1 second timeout (timeout is in ms) */
        char buffer[256];
        ssize_t count = sp_blocking_read(port, buffer, sizeof(buffer) - 1, 1000);

        if (count > 0) {
            buffer[count] = '\0';
            log_line(msg, 0);

            /* Check for "OK" response */
            if (strstr(buffer, "OK") != NULL) {
                snprintf(msg, sizeof(msg), "[SUCCESS] AT command OK at %d baud", baud_rates[i]);
                log_line(msg, 3);
                *found_baud = baud_rates[i];
                sp_close(port);
                sp_free_port(port);
                return 1;
            }
        }
    }

    log_line("[TIMEOUT] No OK response received", 2);
    sp_close(port);
    sp_free_port(port);
    return 0;
}
