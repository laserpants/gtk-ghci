#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _ui ui;

struct _ui
{
    GtkWidget *vbox,
              *hbox,
              *btn,
              *entry,
              *view;
};

ui *init_ui (GtkWidget *window);

G_END_DECLS

#endif /* UI_H */
