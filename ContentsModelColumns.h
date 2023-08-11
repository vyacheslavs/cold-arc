//
// Created by developer on 8/11/23.
//

#ifndef COLD_ARC_GTK_CONTENTSMODELCOLUMNS_H
#define COLD_ARC_GTK_CONTENTSMODELCOLUMNS_H

#include <gtkmm-3.0/gtkmm.h>

class ContentsModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > typ;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<gulong> id;
        Gtk::TreeModelColumn<Glib::ustring> size;
        Gtk::TreeModelColumn<Glib::ustring> hash;

        ContentsModelColumns() {
            add(typ);
            add(name);
            add(id);
            add(size);
            add(hash);
        }
};

#endif //COLD_ARC_GTK_CONTENTSMODELCOLUMNS_H
