//
// Created by developer on 8/11/23.
//

#ifndef COLD_ARC_GTK_MEDIALISTCOLUMNS_H
#define COLD_ARC_GTK_MEDIALISTCOLUMNS_H

#include <gtkmm-3.0/gtkmm.h>

class MediaListColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > active_icon;
        Gtk::TreeModelColumn<gint> checkbox;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> serial;
        Gtk::TreeModelColumn<guint64> capacity;
        Gtk::TreeModelColumn<gulong> id;

        MediaListColumns() {
            add(active_icon);
            add(checkbox);
            add(capacity);
            add(name);
            add(serial);
            add(id);
        }
};

#endif //COLD_ARC_GTK_MEDIALISTCOLUMNS_H
