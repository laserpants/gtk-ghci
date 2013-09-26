#include "ui.h"
#include "commandentry.h"

ui *
init_ui (GtkWidget *window)
{
    GtkWidget *vbox,
              *hbox,
              *btn,
              *scrolled,
              *entry,
              *view;

    ui        *ui_struct;

    PangoFontDescription  *font_desc;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_container_add (GTK_CONTAINER (window), vbox);

    entry = command_entry_new ();

    view = gtk_text_view_new ();
    gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD_CHAR);

    font_desc = pango_font_description_from_string ("Monospace 8");
    gtk_widget_override_font (view, font_desc);
    pango_font_description_free (font_desc);

    font_desc = pango_font_description_from_string ("Monospace 11");
    gtk_widget_override_font (entry, font_desc);
    pango_font_description_free (font_desc);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), view);

    gtk_window_set_default_size (GTK_WINDOW (window), 640, 550);

    btn = gtk_button_new ();
    gtk_button_set_label (GTK_BUTTON (btn), "Run");

    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
    gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    gtk_widget_show_all (window);

    ui_struct        = g_malloc0 (sizeof (ui));
    ui_struct->vbox  = vbox;
    ui_struct->hbox  = hbox;
    ui_struct->btn   = btn;
    ui_struct->entry = entry;
    ui_struct->view  = view;

    //

    command_entry_insert_word (COMMAND_ENTRY (entry), "acosh");
    command_entry_insert_word (COMMAND_ENTRY (entry), "and");
    command_entry_insert_word (COMMAND_ENTRY (entry), "banan");
    command_entry_insert_word (COMMAND_ENTRY (entry), "atan");
    command_entry_insert_word (COMMAND_ENTRY (entry), "appendFile");
    command_entry_insert_word (COMMAND_ENTRY (entry), "asin");
    command_entry_insert_word (COMMAND_ENTRY (entry), "atanh");
    command_entry_insert_word (COMMAND_ENTRY (entry), "abs");

    command_entry_remove_word (COMMAND_ENTRY (entry), "acosh");

    //

    return ui_struct;
}
