#ifndef TABCOMPLETE_H
#define TABCOMPLETE_H

#include <gtk/gtk.h>
#include <string.h>

G_BEGIN_DECLS

typedef struct _completer completer;

struct _completer
{
    GSequence        *wordbank;
    GCompareDataFunc  cmp_func;
};

completer *completer_new ();
void completer_free ();
GList *auto_complete (completer *c, gchar *cmd);

G_END_DECLS

#endif /* TABCOMPLETE_H */
