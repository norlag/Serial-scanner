#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

/* Application-wide data structure */
typedef struct {
    GtkWidget *window;
    GtkWidget *header_bar;
} AppContext;

static void activate_cb(GtkApplication *app, gpointer user_data)
{
    (void)user_data;
    AppContext *ctx = g_new0(AppContext, 1);

    /* Create the main window */
    ctx->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(ctx->window), "Serial Scanner");
    gtk_window_set_default_size(GTK_WINDOW(ctx->window), 800, 600);

    /* Header bar */
    ctx->header_bar = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(ctx->header_bar), "Serial Scanner");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(ctx->header_bar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(ctx->window), ctx->header_bar);

    /* Placeholder label */
    GtkWidget *label = gtk_label_new("Serial Scanner - Cross-Platform Serial Device Utility");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(ctx->window), label);

    gtk_widget_show_all(ctx->window);
}

int main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("com.serialscanner.app",
                                               G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate_cb), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
