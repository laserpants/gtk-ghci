#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "processio.h"

static void
setup_listener (GIOChannel  *channel,
                GSource    **source,
                GSourceFunc  callback,
                gpointer     data)
{
    g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);

    *source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP);

    /* Add the GSource to default context */
    g_source_attach (*source, NULL);
    g_source_set_callback (*source, callback, data, NULL);
}

static void
destroy_listener (GIOChannel  *channel,
                  GSource     *source)
{
    if (source) {
        g_source_destroy (source);
    }
    g_io_channel_shutdown (channel, TRUE, NULL);
    g_io_channel_unref (channel);
}

static void
processio_cleanup (GPid                    pid,
                   gint     G_GNUC_UNUSED  status,
                   pio_env                *env)
{
    destroy_listener (env->io_out, env->src_out);
    destroy_listener (env->io_err, env->src_err);
    destroy_listener (env->io_in, NULL);

    /* Close process, for cross-platform support */
    g_spawn_close_pid (pid);

    /* Destroy app window */
    gtk_widget_destroy (env->window);

    g_byte_array_free (env->read_out, TRUE);
    g_byte_array_free (env->read_err, TRUE);
    g_byte_array_free (env->buffer, TRUE);
    g_free (env);
}

gboolean
processio_init (char          *argv[],
                pio_env      **io_env,
                GSourceFunc    callback,
                gpointer       data)
{
    GError     *error   = NULL;
    gint        in,
                out,
                err;
    GPid        pid;

    GIOChannel *io_out,
               *io_err,
               *io_in;

    GSource    *src_out,
               *src_err;

    /* Launch the process asynchronously */
    g_spawn_async_with_pipes (
              NULL,        /* current working directory */
              argv,        /* child's argument vector */
              NULL,        /* child's environment */
              G_SPAWN_DO_NOT_REAP_CHILD, /* flags from GSpawnFlags */
              NULL,        /* function to run in the child just before exec() */
              NULL,        /* user data for child_setup */
              &pid,        /* child process ID */
              &in,         /* file descriptor to write to child's stdin */
              &out,        /* file descriptor to read child's stdout */
              &err,        /* file descriptor to read child's stderr */
              &error       /* GError structure */
    );

    if (error != NULL) {
        g_warning ("%s", error->message);
        g_error_free (error);

        return FALSE;
    }

    /* Assign a lower process scheduling priority */
    if (!setpriority (PRIO_PROCESS, pid, 5)) {
        g_message ("Changed process priority");
    }

    /* Add watch function to catch termination of the process. This function
     * will clean any remnants of the process.
     *
     * On Unix, using a child watch is equivalent to calling waitpid() or
     * handling the SIGCHLD signal manually.
     */
    g_child_watch_add (pid, (GChildWatchFunc) processio_cleanup, *io_env);

#ifdef G_OS_WIN32
    io_out = g_io_channel_win32_new_fd (out);
    io_err = g_io_channel_win32_new_fd (err);
    io_in  = g_io_channel_win32_new_fd (in);
#else
    io_out = g_io_channel_unix_new (out);
    io_err = g_io_channel_unix_new (err);
    io_in  = g_io_channel_unix_new (in);
#endif

    setup_listener (io_out, &src_out, callback, data);
    setup_listener (io_err, &src_err, callback, data);

    (*io_env)->io_out    = io_out;
    (*io_env)->io_err    = io_err;
    (*io_env)->io_in     = io_in;
    (*io_env)->active    = NULL;
    (*io_env)->src_out   = src_out;
    (*io_env)->src_err   = src_err;
    (*io_env)->pid       = pid;

    return TRUE;
}
