#ifndef PROCESSIO_H
#define PROCESSIO_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TAIL_SIZE 9
#define TAIL_STRING "Prelude> "
#define READ_BUF_SIZE 1024

typedef struct _pio_env pio_env;

struct _pio_env
{
    GtkWidget    *window;

    GIOChannel   *io_out,
                 *io_err,
                 *io_in,
                 *active;       /* Currently active channel or NULL */

    GSource      *src_out,      /* Event sources for stdout and stderr */
                 *src_err;

    GByteArray   *read_out,     /* Read buffers for stdout and stderr */
                 *read_err,
                 *buffer;       /* Auxiliary buffer to use when the other
                                 * stream is active. */
    GPid          pid;
};

gboolean processio_init (char *argv[], pio_env **io_env, GSourceFunc callback, gpointer data);

G_END_DECLS

#endif /* PROCESSIO_H */
