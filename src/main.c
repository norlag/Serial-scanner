#define _POSIX_C_SOURCE 200809L
#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "device.h"
#include "scanner.h"
#include "scan_modes.h"

/* ------------------------------------------------------------------ */
/*  Global application state                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    /* GTK widgets */
    GtkWidget *window;
    GtkWidget *device_list_view;       /* GtkTreeView */
    GtkListStore *device_list_store;   /* model for device list */
    GtkWidget *console_textview;       /* GtkTextView for log console */
    GtkTextBuffer *console_buffer;
    GtkWidget *baud_entry;             /* GtkEntry for manual terminal */
    GtkWidget *scan_mode_combo;        /* GtkComboBoxText for scan mode */
    GtkWidget *status_bar;             /* GtkStatusbar */

    /* State */
    gboolean scan_running;
    gint found_baud;       /* -1 if not found yet */
    gchar *selected_port;  /* currently selected port path */
    Scanner_t *active_scanner;
    DeviceEntry_t *active_device;
} AppState_t;

static AppState_t g_app = {0};

/* ------------------------------------------------------------------ */
/*  Console logging helpers                                            */
/* ------------------------------------------------------------------ */
enum {
    TAG_INFO    = 0,
    TAG_TRY     = 1,
    TAG_TIMEOUT = 2,
    TAG_SUCCESS = 3,
    TAG_ERROR   = 4,
    TAG_COUNT
};

static GtkTextTag *g_tags[TAG_COUNT];

static void console_init(GtkTextBuffer *buf) {
    g_tags[TAG_INFO]    = gtk_text_buffer_create_tag(buf, "info",    "foreground", "gray",   NULL);
    g_tags[TAG_TRY]     = gtk_text_buffer_create_tag(buf, "try",     "foreground", "blue",   NULL);
    g_tags[TAG_TIMEOUT] = gtk_text_buffer_create_tag(buf, "timeout", "foreground", "red",    NULL);
    g_tags[TAG_SUCCESS] = gtk_text_buffer_create_tag(buf, "success", "foreground", "green",  NULL);
    g_tags[TAG_ERROR]   = gtk_text_buffer_create_tag(buf, "error",   "foreground", "red",    "weight", PANGO_WEIGHT_BOLD, NULL);
}

static void console_append(const char *text, int tag_id) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(g_app.console_buffer, &iter);
    gtk_text_buffer_insert_with_tags(g_app.console_buffer, &iter, text, -1,
                                     g_tags[tag_id], NULL);
    gtk_text_buffer_insert(g_app.console_buffer, &iter, "\n", -1);

    /* Auto-scroll to bottom */
    GtkTextMark *mark = gtk_text_buffer_get_mark(g_app.console_buffer, "end");
    if (mark) {
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(g_app.console_textview), mark, 0.0, 0.0, 0.0, 0.0);
    }
}

/* Thread-safe log from background thread via g_idle_add */
typedef struct {
    gchar *message;
    gint tag;
} LogEntry_t;

static gboolean log_idle_cb(gpointer user_data) {
    LogEntry_t *entry = (LogEntry_t *)user_data;
    console_append(entry->message, entry->tag);
    g_free(entry->message);
    g_free(entry);
    return G_SOURCE_REMOVE;
}

static void log_to_console(const char *message, int tag) {
    LogEntry_t *entry = g_new0(LogEntry_t, 1);
    entry->message = g_strdup(message);
    entry->tag = tag;
    g_idle_add(log_idle_cb, entry);
}

/* ------------------------------------------------------------------ */
/*  Device list management                                             */
/* ------------------------------------------------------------------ */
static void refresh_devices(gpointer user_data) {
    (void)user_data;
    if (g_app.scan_running) return;

    size_t count = 0;
    DeviceEntry_t *list = device_list_enumerate(&count);

    gtk_list_store_clear(g_app.device_list_store);

    if (!list) {
        console_append("No serial ports detected", TAG_INFO);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(g_app.device_list_store, &iter);
        gtk_list_store_set(g_app.device_list_store, &iter,
                           0, list[i].name ? list[i].name : "(unknown)",
                           1, list[i].path ? list[i].path : "(unknown)",
                           2, list[i].manufacturer ? list[i].manufacturer : "N/A",
                           3, list[i].description ? list[i].description : "Serial Port",
                           -1);
    }

    g_app.selected_port = g_strdup(list[0].path);
    device_list_free(&list, count);

    char msg[256];
    snprintf(msg, sizeof(msg), "Found %zu device(s)", count);
    console_append(msg, TAG_INFO);
}

/* ------------------------------------------------------------------ */
/*  Scan mode callbacks                                                */
/* ------------------------------------------------------------------ */
static void ui_log_line(const char *line, int color_tag) {
    log_to_console(line, color_tag);
}

static void ui_on_success(const char *port, int baud) {
    (void)port;
    g_app.found_baud = baud;
    char msg[256];
    snprintf(msg, sizeof(msg), "SUCCESS: Device found on %s at %d baud", port, baud);
    log_to_console(msg, TAG_SUCCESS);

    /* Unlock the baud entry */
    gtk_widget_set_sensitive(g_app.baud_entry, TRUE);
    gtk_entry_set_text(GTK_ENTRY(g_app.baud_entry), "115200");
}

/* ------------------------------------------------------------------ */
/*  Scan button handler                                                */
/* ------------------------------------------------------------------ */
static void on_scan_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;

    if (g_app.scan_running) {
        /* Stop scan */
        scanner_stop(g_app.active_scanner);
        g_app.scan_running = FALSE;
        gtk_button_set_label(GTK_BUTTON(widget), "Scan");
        console_append("Scan stopped by user", TAG_INFO);
        return;
    }

    /* Get selected port */
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_app.device_list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *port_path = NULL;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gtk_tree_model_get(model, &iter, 1, &port_path, -1);
    }

    if (!port_path || strlen(port_path) == 0) {
        console_append("No port selected", TAG_ERROR);
        return;
    }

    /* Get scan mode */
    int mode = gtk_combo_box_get_active(GTK_COMBO_BOX(g_app.scan_mode_combo)); /* 0=echo, 1=passive, 2=AT */

    /* Create scanner */
    g_app.active_scanner = scanner_create(port_path, mode);
    if (!g_app.active_scanner) {
        console_append("Failed to create scanner", TAG_ERROR);
        return;
    }

    scanner_set_ui_callbacks(g_app.active_scanner, ui_log_line, ui_on_success);

    /* Reset state */
    g_app.found_baud = -1;
    g_app.scan_running = TRUE;
    gtk_button_set_label(GTK_BUTTON(widget), "Stop");
    gtk_widget_set_sensitive(g_app.device_list_view, FALSE);

    char msg[256];
    snprintf(msg, sizeof(msg), "Starting scan on %s...", port_path);
    console_append(msg, TAG_TRY);

    /* Start scan thread */
    scanner_start(g_app.active_scanner);
}

/* ------------------------------------------------------------------ */
/*  Terminal send handler                                              */
/* ------------------------------------------------------------------ */
static void on_terminal_activate(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;

    const gchar *text = gtk_entry_get_text(GTK_ENTRY(g_app.baud_entry));
    if (!text || strlen(text) == 0) return;

    char msg[512];
    snprintf(msg, sizeof(msg), "TX: %s", text);
    console_append(msg, TAG_TRY);

    /* Echo back in terminal mode */
    snprintf(msg, sizeof(msg), "RX: (echo) %s", text);
    log_to_console(msg, TAG_INFO);
}

/* ------------------------------------------------------------------ */
/*  Window destroy handler                                             */
/* ------------------------------------------------------------------ */
static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;

    /* Cleanup */
    if (g_app.active_scanner) {
        scanner_stop(g_app.active_scanner);
        scanner_destroy(&g_app.active_scanner);
    }
    if (g_app.active_device) {
        device_close(g_app.active_device);
    }
    g_free(g_app.selected_port);

    gtk_main_quit();
}

/* ------------------------------------------------------------------ */
/*  Main application setup                                             */
/* ------------------------------------------------------------------ */
static void build_ui(void) {
    /* --- Main window --- */
    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_app.window), "Serial Scanner");
    gtk_window_set_default_size(GTK_WINDOW(g_app.window), 900, 650);
    gtk_container_set_border_width(GTK_CONTAINER(g_app.window), 10);
    g_signal_connect(g_app.window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    /* --- Main vertical box --- */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(g_app.window), vbox);

    /* --- Top toolbar --- */
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    GtkWidget *btn_refresh = gtk_button_new_with_label("Refresh");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(refresh_devices), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_refresh, FALSE, FALSE, 0);

    g_app.scan_mode_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_app.scan_mode_combo), "echo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_app.scan_mode_combo), "passive");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_app.scan_mode_combo), "at");
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_app.scan_mode_combo), 0);
    gtk_box_pack_start(GTK_BOX(toolbar), g_app.scan_mode_combo, FALSE, FALSE, 0);

    GtkWidget *btn_scan = gtk_button_new_with_label("Scan");
    g_signal_connect(btn_scan, "clicked", G_CALLBACK(on_scan_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(toolbar), btn_scan, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    /* --- Main content area (horizontal panes) --- */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    /* --- Left pane: Device list --- */
    GtkWidget *frame_left = gtk_frame_new("Serial Devices");
    gtk_box_pack_start(GTK_BOX(hbox), frame_left, FALSE, TRUE, 0);

    g_app.device_list_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    g_app.device_list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_app.device_list_store));
    g_object_unref(g_app.device_list_store);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col;

    col = gtk_tree_view_column_new_with_attributes("Port Name", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.device_list_view), col);

    col = gtk_tree_view_column_new_with_attributes("Path", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.device_list_view), col);

    col = gtk_tree_view_column_new_with_attributes("Manufacturer", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.device_list_view), col);

    col = gtk_tree_view_column_new_with_attributes("Description", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.device_list_view), col);

    GtkWidget *scroll_devices = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_devices), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll_devices), g_app.device_list_view);

    gtk_container_add(GTK_CONTAINER(frame_left), scroll_devices);

    /* --- Right pane: Console --- */
    GtkWidget *frame_right = gtk_frame_new("Console");
    gtk_box_pack_start(GTK_BOX(hbox), frame_right, TRUE, TRUE, 0);

    g_app.console_buffer = gtk_text_buffer_new(NULL);
    console_init(g_app.console_buffer);

    g_app.console_textview = gtk_text_view_new_with_buffer(g_app.console_buffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(g_app.console_textview), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(g_app.console_textview), FALSE);

    GtkWidget *scroll_console = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_console), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(scroll_console), g_app.console_textview);

    gtk_container_add(GTK_CONTAINER(frame_right), scroll_console);

    /* --- Bottom: Terminal entry --- */
    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    GtkWidget *lbl = gtk_label_new("Terminal:");
    gtk_box_pack_start(GTK_BOX(hbox_bottom), lbl, FALSE, FALSE, 0);

    g_app.baud_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_app.baud_entry), "Enter command and press Enter...");
    gtk_entry_set_visibility(GTK_ENTRY(g_app.baud_entry), TRUE);
    gtk_widget_set_sensitive(g_app.baud_entry, FALSE); /* locked until scan finds device */
    g_signal_connect(g_app.baud_entry, "activate", G_CALLBACK(on_terminal_activate), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_bottom), g_app.baud_entry, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 0);

    /* --- Status bar --- */
    g_app.status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), g_app.status_bar, FALSE, FALSE, 0);

    /* Initial device list */
    refresh_devices(NULL);
}

int main(int argc, char *argv[]) {
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    gtk_init(&argc, &argv);

    build_ui();

    gtk_widget_show_all(g_app.window);

    gtk_main();
    return 0;
}
