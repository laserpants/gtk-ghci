#include <string.h>
#include "commandentry.h"

struct _CommandEntryPrivate
{
    GList       *history,
                *commands;
    GSequence   *wordbank;
    GCompareDataFunc cmp_func;
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

static void
list_append (gchar *str, GList **list)
{
    *list = g_list_prepend (*list, g_strdup (str));
}

static GList *
auto_complete (CommandEntry *self, gchar *cmd)
{
    CommandEntryPrivate *priv = self->priv;
    GList               *str_list;
    GSequenceIter       *begin_iter,
                        *end_iter;
    gchar               *dup;
    size_t               len;

    begin_iter = g_sequence_lookup (priv->wordbank, cmd, priv->cmp_func, NULL);
    if (!begin_iter) {
        begin_iter = g_sequence_search (priv->wordbank, cmd, priv->cmp_func, NULL);
    }

    dup = g_strdup (cmd);
    len = strlen (cmd);
    ++dup[len - 1];

    end_iter = g_sequence_search (priv->wordbank, dup, priv->cmp_func, NULL);
    g_free (dup);

    if (begin_iter == end_iter) {
        return NULL;
    }

    str_list = g_list_alloc ();
    g_sequence_foreach_range (begin_iter, end_iter,
                              (GFunc) list_append,
                              &str_list);
    return str_list;
}

static void
collect (gchar *needle, GString **haystack)
{
    if (needle) {
        *haystack = g_string_append (*haystack, needle);
        if ('\n' != *needle)
            *haystack = g_string_append (*haystack, "\t");
    }
}

static void
on_tab (CommandEntry *entry)
{
    GList *res;
    gchar *input;
    guint  listlen;

    g_signal_emit_by_name (entry, "tab-press");

    if (!gtk_entry_get_text_length (GTK_ENTRY (entry))) {
        return;
    }

    /* Attempt auto-complete */
    input = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    res = auto_complete (entry, input);
    if (res) {
        listlen = g_list_length (res);
        if (2 == listlen) {
            gtk_entry_set_text (GTK_ENTRY (entry), g_list_first (res)->data);
            gtk_editable_set_position (GTK_EDITABLE (entry), -1);
        } else {
            GString *str = g_string_new (NULL);
            g_list_foreach (res, (GFunc) collect, &str);
            str = g_string_append (str, "\n");
            g_signal_emit_by_name (entry, "auto-complete", str);
            g_string_free (str, TRUE);
        }
        g_list_free_full (res, g_free);
    }

    g_free (input);
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
        on_tab (self);
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

    _signals[AUTO_COMPLETE] =
            g_signal_new ("auto-complete",
                          G_OBJECT_CLASS_TYPE (gobject_class),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (CommandEntryClass, auto_complete),
                          NULL, NULL,
                          NULL,
                          G_TYPE_NONE, 1, G_TYPE_POINTER);
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
    priv->wordbank = g_sequence_new (g_free);
    priv->cmp_func = (GCompareDataFunc) g_strcmp0;
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
    if (priv->wordbank) {
        g_sequence_free (priv->wordbank);
    }

    priv->commands = NULL;
    priv->history  = NULL;
    priv->wordbank = NULL;
    priv->cmp_func = NULL;

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

void
command_entry_insert_word (CommandEntry *self, gchar *word)
{
    CommandEntryPrivate *priv = COMMAND_ENTRY_GET_PRIVATE (self);

    g_sequence_insert_sorted (priv->wordbank, g_strdup (word),
                              priv->cmp_func, NULL);
}

void
command_entry_remove_word (CommandEntry *self, gchar *word)
{
    GSequenceIter *iter;
    CommandEntryPrivate *priv = COMMAND_ENTRY_GET_PRIVATE (self);

    iter = g_sequence_lookup (priv->wordbank, word, priv->cmp_func, NULL);

    if (iter) {
        g_sequence_remove (iter);
    }
}
