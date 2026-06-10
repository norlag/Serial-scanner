#define _POSIX_C_SOURCE 200809L
#include "scanner.h"
#include "device.h"
#include "scan_modes.h"
#include "libserialport.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/* Forward declaration */
static void *scanner_thread_func(void *arg);

/* ------------------------------------------------------------------ */
/*  Callback wrapper for g_idle_add                                    */
/* ------------------------------------------------------------------ */
typedef struct {
    void (*cb)(const char *, int);
    const char *port;
    int baud;
} SuccessData_t;

static gboolean success_idle_cb(gpointer user_data) {
    SuccessData_t *data = (SuccessData_t *)user_data;
    data->cb(data->port, data->baud);
    g_free((void *)data->port);
    g_free(data);
    return G_SOURCE_REMOVE;
}

/* ------------------------------------------------------------------ */
/*  Constructor / Destructor                                           */
/* ------------------------------------------------------------------ */
struct Scanner *scanner_create(const char *port_name, ScanMode_t mode) {
    if (!port_name) return NULL;

    struct Scanner *s = calloc(1, sizeof(struct Scanner));
    if (!s) return NULL;

    s->port_name = strdup(port_name);
    s->mode = mode;
    s->running = 0;
    s->state = SCANNER_STATE_IDLE;
    s->found_baud = -1;
    return s;
}

void scanner_destroy(struct Scanner **scanner) {
    if (!scanner || !*scanner) return;
    struct Scanner *s = *scanner;
    if (s->running) scanner_stop(s);
    free(s->port_name);
    free(s);
    *scanner = NULL;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
int scanner_start(struct Scanner *scanner) {
    if (!scanner || scanner->running) return -1;
    scanner->running = 1;
    scanner->state = SCANNER_STATE_SCANNING;
    int ret = pthread_create(&scanner->thread, NULL, scanner_thread_func, scanner);
    if (ret != 0) {
        scanner->running = 0;
        scanner->state = SCANNER_STATE_IDLE;
        return -1;
    }
    return 0;
}

int scanner_stop(struct Scanner *scanner) {
    if (!scanner || !scanner->running) return -1;
    scanner->running = 0;
    return pthread_cancel(scanner->thread);
}

void scanner_set_mode(struct Scanner *scanner, ScanMode_t mode) {
    if (!scanner) return;
    scanner->mode = mode;
}

void scanner_set_ui_callbacks(struct Scanner *scanner, scan_callback_t log_line,
                              void (*on_success)(const char *port, int baud)) {
    if (!scanner) return;
    scanner->log_line = log_line;
    scanner->on_success = on_success;
}

/* ------------------------------------------------------------------ */
/*  Worker thread                                                      */
/* ------------------------------------------------------------------ */
static void *scanner_thread_func(void *arg) {
    struct Scanner *scanner = (struct Scanner *)arg;
    int found = 0;
    int local_baud = -1;

    switch (scanner->mode) {
        case SCAN_ECHO:
            found = scan_echo_mode(scanner->port_name, scanner->log_line, &local_baud);
            break;
        case SCAN_PASSIVE:
            found = scan_passive_mode(scanner->port_name, scanner->log_line, &local_baud);
            break;
        case SCAN_AT:
            found = scan_at_mode(scanner->port_name, scanner->log_line, &local_baud);
            break;
    }

    if (found) {
        scanner->found_baud = local_baud;
        scanner->state = SCANNER_STATE_COMPLETE;
        if (scanner->on_success) {
            SuccessData_t *data = g_new0(SuccessData_t, 1);
            data->cb = scanner->on_success;
            data->port = strdup(scanner->port_name);
            data->baud = local_baud;
            g_idle_add(success_idle_cb, data);
        }
    }

    scanner->running = 0;
    scanner->state = SCANNER_STATE_COMPLETE;
    return NULL;
}
