#include "tabcomplete.h"

static void
list_append (gchar    *str,
             GList   **list)
{
    *list = g_list_prepend (*list, g_strdup (str));
}

completer *
completer_new ()
{
    completer *c;
    c = g_malloc0 (sizeof (completer));
    c->wordbank = g_sequence_new (g_free);
    c->cmp_func = (GCompareDataFunc) g_strcmp0;

    g_sequence_insert_sorted (c->wordbank, g_strdup ("acosh"), c->cmp_func, NULL);
    g_sequence_insert_sorted (c->wordbank, g_strdup ("and"), c->cmp_func, NULL);
    g_sequence_insert_sorted (c->wordbank, g_strdup ("banan"), c->cmp_func, NULL);
    g_sequence_insert_sorted (c->wordbank, g_strdup ("atan"), c->cmp_func, NULL);
    g_sequence_insert_sorted (c->wordbank, g_strdup ("appendFile"), c->cmp_func, NULL);
    g_sequence_insert_sorted (c->wordbank, g_strdup ("asin"), c->cmp_func, NULL);
    g_sequence_insert_sorted (c->wordbank, g_strdup ("atanh"), c->cmp_func, NULL);
    g_sequence_insert_sorted (c->wordbank, g_strdup ("abs"), c->cmp_func, NULL);

    return c;
}

void
completer_free (completer *c)
{
    if (c) {
        g_sequence_free (c->wordbank);
        g_free (c);
    }
}

GList *
auto_complete (completer *c,
               gchar     *cmd)
{
    GList          *str_list;
    GSequenceIter  *begin_iter,
                   *end_iter;
    gchar          *dup;
    size_t          len;

    begin_iter = g_sequence_lookup (c->wordbank, cmd, c->cmp_func, NULL);
    if (!begin_iter) {
        begin_iter = g_sequence_search (c->wordbank, cmd, c->cmp_func, NULL);
    }

    dup = g_strdup (cmd);
    len = strlen (cmd);
    ++dup[len - 1];

    end_iter = g_sequence_search (c->wordbank, dup, c->cmp_func, NULL);
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
