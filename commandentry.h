#ifndef COMMANDENTRY_H
#define COMMANDENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_COMMAND_ENTRY             (command_entry_get_type ())
#define COMMAND_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_COMMAND_ENTRY, CommandEntry))
#define COMMAND_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_COMMAND_ENTRY, CommandEntryClass))
#define IS_COMMAND_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_COMMAND_ENTRY))
#define IS_COMMAND_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_COMMAND_ENTRY))
#define COMMAND_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_COMMAND_ENTRY, CommandEntryClass))
#define COMMAND_ENTRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_COMMAND_ENTRY, CommandEntryPrivate))

typedef struct _CommandEntry           CommandEntry;
typedef struct _CommandEntryPrivate    CommandEntryPrivate;
typedef struct _CommandEntryClass      CommandEntryClass;

enum {
    ENTER_PRESS,
    TAB_PRESS,
    AUTO_COMPLETE,
    LAST_SIGNAL
};

struct _CommandEntry
{
    /*< private >*/
    GtkEntry  parent_instance;

    CommandEntryPrivate *priv;
};

struct _CommandEntryClass
{
    GtkEntryClass parent_class;

    void (* enter_press)   (CommandEntry *entry);
    void (* tab_press)     (CommandEntry *entry);
    void (* auto_complete) (CommandEntry *entry);
};

GType       command_entry_get_type     (void) G_GNUC_CONST;
GtkWidget  *command_entry_new          (void);
void        command_entry_insert_word  (CommandEntry *self, gchar *word);
void        command_entry_remove_word  (CommandEntry *self, gchar *word);

G_END_DECLS

#endif /* COMMANDENTRY_H */
