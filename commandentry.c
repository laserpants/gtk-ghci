#include "commandentry.h"

struct _CommandEntryPrivate
{
    GList       *history,
                *commands;
    gint         index,
                 count;
};

static guint _signals[LAST_SIGNAL] = { 0 };

static void command_entry_dispose     (GObject *object);
static void command_entry_finalize    (GObject *object);
static void command_entry_destroy     (GtkWidget *widget);

G_DEFINE_TYPE (CommandEntry, command_entry, GTK_TYPE_ENTRY)

static void
do_append (gchar *str, GList **list)
{
    *list = g_list_append (*list, g_strdup (str));
}

static void
update_entry_text (GtkEntry *entry, const gchar *text)
{
    gtk_entry_set_text (entry, text ? text : "");
    gtk_editable_set_position (GTK_EDITABLE (entry), -1);
}

static void
command_entry_navigate (CommandEntry *entry, gboolean up)
{
    GList               *list;
    CommandEntryPrivate *priv = entry->priv;

    if (NULL == priv->commands) {
        priv->commands = g_list_append (priv->commands, NULL);
    }
    list = g_list_nth (priv->commands, priv->index);
    if (list->data) {
        g_free (list->data);
    }

    list->data = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
    priv->index += up ? 1 : -1;
    update_entry_text (GTK_ENTRY (entry),
                       g_list_nth_data (priv->commands, priv->index));
}

static gboolean
on_key_pressed (GtkEntry                  *entry,
                GdkEventKey               *event,
                gpointer    G_GNUC_UNUSED  data)
{
    CommandEntry *self = COMMAND_ENTRY (entry);
    CommandEntryPrivate *priv = self->priv;

    switch (event->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
        priv->history = g_list_prepend (priv->history,
            g_strdup (gtk_entry_get_text (GTK_ENTRY (entry))));

        /* Deep copy the history list to priv->commands */
        g_list_free_full (priv->commands, (GDestroyNotify) g_free);
        priv->commands = NULL;
        g_list_foreach (priv->history, (GFunc) do_append, &priv->commands);
        priv->index = 0;
        priv->commands = g_list_prepend (priv->commands, NULL);
        priv->count++;
        g_signal_emit_by_name (entry, "enter-press");

        /* Clear the text input */
        gtk_entry_set_text (entry, "");

        return TRUE;
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
        if (priv->index + 1 < priv->count) {
            command_entry_navigate (self, TRUE);
        }
        return TRUE;
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
        if (priv->index) {
            command_entry_navigate (self, FALSE);
        }
        return TRUE;
    case GDK_KEY_Tab:
        g_signal_emit_by_name (entry, "tab-press");
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/**
 * Class initialization: Used to add properties, hook finalization functions,
 * add the private structure to the class and initialize the parent class
 * pointer.
 */
static void
command_entry_class_init (CommandEntryClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class  = GTK_WIDGET_CLASS (klass);

    /* Add private structure */
    g_type_class_add_private (klass, sizeof (CommandEntryPrivate));

    gobject_class->dispose  = command_entry_dispose;
    gobject_class->finalize = command_entry_finalize;

    widget_class->destroy = command_entry_destroy;

    klass->enter_press = NULL;

    _signals[ENTER_PRESS] =
            g_signal_new ("enter-press",
                          G_OBJECT_CLASS_TYPE (gobject_class),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (CommandEntryClass, enter_press),
                          NULL, NULL,
                          NULL,
                          G_TYPE_NONE, 0);

    _signals[TAB_PRESS] =
            g_signal_new ("tab-press",
                          G_OBJECT_CLASS_TYPE (gobject_class),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (CommandEntryClass, tab_press),
                          NULL, NULL,
                          NULL,
                          G_TYPE_NONE, 0);
}

/**
 * Constructor: Initialization of instance public and private data.
 */
static void
command_entry_init (CommandEntry *self)
{
    CommandEntryPrivate *priv;

    self->priv = COMMAND_ENTRY_GET_PRIVATE (self);
    priv = self->priv;

    g_signal_connect (G_OBJECT (self), "key-press-event",
                      G_CALLBACK (on_key_pressed),
                      NULL);

    priv->history  = NULL;
    priv->commands = NULL;
    priv->index    = 0;
    priv->count    = 1;
}

/**
 * Destructor
 */
static void
command_entry_dispose (GObject *object)
{
    CommandEntry *self = COMMAND_ENTRY (object);
    CommandEntryPrivate *priv;

    priv = COMMAND_ENTRY_GET_PRIVATE (self);

    g_list_free_full (priv->commands, (GDestroyNotify) g_free);
    g_list_free_full (priv->history,  (GDestroyNotify) g_free);

    priv->commands = NULL;
    priv->history  = NULL;

    G_OBJECT_CLASS (command_entry_parent_class)->dispose (object);
}

static void
command_entry_finalize (GObject *object)
{
    G_OBJECT_CLASS (command_entry_parent_class)->finalize (object);
}

static void
command_entry_destroy (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (command_entry_parent_class)->destroy (widget);
}

GtkWidget *
command_entry_new (void)
{
    return g_object_new (TYPE_COMMAND_ENTRY, NULL);
}
