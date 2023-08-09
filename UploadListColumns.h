//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADLISTCOLUMNS_H
#define COLD_ARC_GTK_UPLOADLISTCOLUMNS_H

#include <gtkmm-3.0/gtkmm.h>

class UploadListColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > status;
        Gtk::TreeModelColumn<Glib::ustring> path;
        Gtk::TreeModelColumn<Glib::ustring> reason;

        UploadListColumns() {
            add(status);
            add(path);
            add(reason);
        }
};

#endif //COLD_ARC_GTK_UPLOADLISTCOLUMNS_H
