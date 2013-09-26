#include <gtk/gtk.h>
#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include "processio.h"
#include "tabcomplete.h"
#include "ui.h"

typedef struct _app app;

struct _app
{
    GtkWidget    *window;
    pio_env      *io_env;
    struct _ui   *ui;
    completer    *autocompl;
    guchar        tail[TAIL_SIZE];
    guint8        tlen;
    gboolean      ctrlc;
};

static gboolean
on_window_destroy (GtkWidget G_GNUC_UNUSED *object,
                   GdkEvent  G_GNUC_UNUSED *event,
                   app                     *obj)
{
    if (!kill (obj->io_env->pid, SIGKILL)) {
        g_message ("SIGKILL");
    }

    completer_free (obj->autocompl);
    g_free (obj->ui);
    g_free (obj);

    return TRUE;
}

static void G_GNUC_UNUSED
io_write (GIOChannel   *channel,
          const gchar  *data)
{
    g_io_channel_write_chars (channel, data, -1, NULL, NULL);
    g_io_channel_flush (channel, NULL);
}

static gboolean
on_key_pressed (GtkWidget   G_GNUC_UNUSED *object,
                GdkEventKey G_GNUC_UNUSED *event,
                app                       *obj)
{
    switch (event->keyval)
    {
    case GDK_KEY_C:
    case GDK_KEY_c:
        if (event->state & GDK_CONTROL_MASK) {
            if (!kill (obj->io_env->pid, SIGINT)) {
                obj->ctrlc = TRUE;
                g_message ("SIGINT");
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void
on_button_clicked (GtkWidget  G_GNUC_UNUSED *button,
                   app                      *obj)
{
    const gchar *text = gtk_entry_get_text (GTK_ENTRY (obj->ui->entry));

    if (*text) {
        GtkTextIter    iter;
        GtkTextBuffer *buffer;

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (obj->ui->view));

        gtk_text_buffer_get_end_iter (buffer, &iter);
        gtk_text_buffer_insert (buffer, &iter, text, strlen (text));
        gtk_text_buffer_insert (buffer, &iter, "\n", 1);

        g_io_channel_write_chars (obj->io_env->io_in, text, -1, NULL, NULL);
        if (!g_str_has_suffix (text, "\n")) {
            g_io_channel_write_chars (obj->io_env->io_in, "\n", 1, NULL, NULL);
        }
        g_io_channel_flush (obj->io_env->io_in, NULL);

        g_io_channel_flush (obj->io_env->io_out, NULL);
        g_io_channel_flush (obj->io_env->io_err, NULL);

        obj->ctrlc = FALSE;
        gtk_entry_set_text (GTK_ENTRY (obj->ui->entry), "");
    }
}

static void
scroll_to_end (GtkTextView *view)
{
    GtkAdjustment *adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (view));

    gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj));
    gtk_scrollable_set_vadjustment (GTK_SCROLLABLE (view), adj);

    while (gtk_events_pending ()) {
        gtk_main_iteration();
    }
}

static void
print_out (GtkTextView   *view,
           guint8        *data,
           gsize          bytes)
{
    GtkTextBuffer *buffer;
    GtkTextIter    iter;

    buffer = gtk_text_view_get_buffer (view);

    gtk_text_buffer_get_end_iter (buffer, &iter);
    gtk_text_buffer_insert (buffer, &iter, (const gchar *) data, bytes);

    scroll_to_end (view);

    gtk_text_buffer_get_end_iter (buffer, &iter);
    scroll_to_end (view);
}

static void
process (app    *obj,
         gsize   bytes,
         gchar  *data)
{
    gsize s = TAIL_SIZE - obj->tlen;

    if (s && bytes <= s) {
        /* Append everything to tail */
        memcpy (obj->tail + obj->tlen, data, bytes);
        obj->tlen += bytes;
        g_assert (obj->tlen <= TAIL_SIZE);
    } else {
        guint8 t = obj->tlen;
        gsize  x = MIN (t, t + bytes - TAIL_SIZE);

        if (x) {
            /* Flush out x first characters currently in tail buffer */
            print_out (GTK_TEXT_VIEW (obj->ui->view),
                       obj->tail, x);

            if (t && x < t) {
                /* Shift tail buffer to the left x steps */
                guint8 i;
                for (i = 0; i != (t - x); ++i)
                    obj->tail[i] = obj->tail[i + x];

                obj->tlen -= x;
            } else {
                obj->tlen  = 0;
            }
        }

        s = TAIL_SIZE - obj->tlen;
        /* Flush out read buffer, but leave enough for tail... */
        print_out (GTK_TEXT_VIEW (obj->ui->view),
                   (guint8 *) data, bytes - s);

        if (s) {
            g_assert (obj->tlen + s <= TAIL_SIZE);

            /* Append the rest to tail */
            memcpy (obj->tail + obj->tlen,
                    data + bytes - s, s);

            obj->tlen += s;
        }
    }
}

static gboolean
io_read (GIOChannel     *channel,
         GIOCondition    cond,
         app            *obj)
{
    gsize          bytes;
    GByteArray    *read_buf;

    if (cond == G_IO_HUP) {
        return FALSE;
    }

    read_buf = (obj->io_env->io_out == channel) ? obj->io_env->read_out
                                                : obj->io_env->read_err;

    if (G_IO_STATUS_ERROR != g_io_channel_read_chars (channel,
                                  (gchar *) read_buf->data,
                                  READ_BUF_SIZE, &bytes, NULL))
    {
        if (obj->ctrlc) {
            while (g_io_channel_get_buffer_condition (channel) & G_IO_IN) {
                g_io_channel_flush (channel, NULL);
                g_usleep(500);
            }

            obj->tlen = 0;
            return TRUE;
        }

        if (bytes) {

            /* -->
            read_buf->data[bytes] = '\0';
            g_message ("%s", read_buf->data);
            <-- */

            if (!obj->io_env->active) {
                /* Set this channel as active */
                obj->io_env->active = channel;
            } else if (obj->io_env->active != channel) {
                g_byte_array_append (obj->io_env->buffer, read_buf->data, bytes);

                /* Keep UI responsive */
                while (gtk_events_pending ()) {
                    gtk_main_iteration();
                }

                return TRUE;
            }

            process (obj, bytes, (gchar *) read_buf->data);

            if ((TAIL_SIZE == obj->tlen &&
                !strncmp ((const gchar *) obj->tail, TAIL_STRING, TAIL_SIZE))
             || (obj->io_env->io_err == obj->io_env->active &&
                 obj->tlen && '\n' == obj->tail[obj->tlen - 1]))
            {
                if (obj->io_env->io_err == obj->io_env->active) {
                    print_out (GTK_TEXT_VIEW (obj->ui->view),
                               obj->tail, obj->tlen);
                }

                if (obj->io_env->buffer->len) {
                    g_byte_array_free (obj->io_env->buffer, TRUE);
                    obj->io_env->buffer = g_byte_array_new ();
                }

                obj->tlen = 0;
                obj->io_env->active = NULL;
            }
        }

        /* Keep UI responsive */
        while (gtk_events_pending ()) {
            gtk_main_iteration();
        }

        return TRUE;
    }

    return FALSE;
}

static void
print_to_view (gchar *str,
               app   *obj)
{
    GtkTextView *view = GTK_TEXT_VIEW (obj->ui->view);
    if (str) {
        print_out (view, (guint8 *) str, strlen (str));
        if ('\n' != *str)
            print_out (view, (guint8 *) "\t", 1);
    }
}

static void
on_tab (GtkWidget  G_GNUC_UNUSED *widget,
        app                      *obj)
{
    GList *res;
    gchar *input;
    guint  listlen;

    /* Attempt auto-complete */
    input = g_strdup (gtk_entry_get_text (GTK_ENTRY (obj->ui->entry)));

    res = auto_complete (obj->autocompl, input);
    if (res) {
        listlen = g_list_length (res);
        if (2 == listlen) {
            gtk_entry_set_text (GTK_ENTRY (obj->ui->entry),
                                g_list_first(res)->data);

            gtk_editable_set_position (GTK_EDITABLE (obj->ui->entry), -1);
        } else {
            g_list_foreach (res, (GFunc) print_to_view, obj);
            print_to_view ("\n", obj);
        }
        g_list_free_full (res, g_free);
    }

    g_free (input);
}

static void
activate (GtkApplication                *application,
          gpointer        G_GNUC_UNUSED  user_data)
{
    gchar     **args;
    app        *obj;

    obj         = g_malloc0 (sizeof (app));
    obj->io_env = g_malloc0 (sizeof (pio_env));
    obj->window = gtk_application_window_new (application);
    obj->autocompl = completer_new ();

    /* Initialize child process */
    args = g_malloc_n (5, sizeof (gchar *));

    args[0] = g_strdup ("/usr/lib/ghc/lib/ghc");
    args[1] = g_strdup ("-B/usr/lib/ghc");
    args[2] = g_strdup ("--interactive");
    args[3] = g_strdup ("-ignore-dot-ghci");
    args[4] = NULL;

    obj->io_env->window   = obj->window;
    obj->io_env->read_out = g_byte_array_sized_new (READ_BUF_SIZE);
    obj->io_env->read_err = g_byte_array_sized_new (READ_BUF_SIZE);
    obj->io_env->buffer   = g_byte_array_new ();

    if (!processio_init (args, &obj->io_env, (GSourceFunc) io_read, obj)) {
        g_error ("Failed to launch ghci process.");
    }

    g_strfreev (args);

    obj->ui = init_ui (obj->window);

    g_signal_connect (G_OBJECT (obj->window), "delete-event",
                      G_CALLBACK (on_window_destroy),
                      obj);

    g_signal_connect (G_OBJECT (obj->window), "key-press-event",
                      G_CALLBACK (on_key_pressed),
                      obj);

    g_signal_connect (G_OBJECT (obj->ui->btn), "clicked",
                      G_CALLBACK (on_button_clicked),
                      obj);

    g_signal_connect (G_OBJECT (obj->ui->entry), "enter-press",
                      G_CALLBACK (on_button_clicked),
                      obj);

    g_signal_connect (G_OBJECT (obj->ui->entry), "tab-press",
                      G_CALLBACK (on_tab),
                      obj);
}

int
main (int argc, char **argv)
{
    GtkApplication *app;
    int status;
    
    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    
    g_message ("Exiting gracefully...");

    return status;
}
