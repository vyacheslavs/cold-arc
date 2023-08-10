//
// Created by developer on 8/8/23.
//

#ifndef COLD_ARC_GTK_FOLDERMODELCOLUMNS_H
#define COLD_ARC_GTK_FOLDERMODELCOLUMNS_H

#include <gtkmm-3.0/gtkmm.h>

class FolderModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > status;
        Gtk::TreeModelColumn<Glib::ustring> folder;
        Gtk::TreeModelColumn<gulong> id;

        FolderModelColumns() {
            add(status);
            add(folder);
            add(id);
        }
};


#endif //COLD_ARC_GTK_FOLDERMODELCOLUMNS_H
