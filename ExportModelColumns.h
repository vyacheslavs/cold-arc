//
// Created by developer on 9/10/23.
//

#ifndef COLD_ARC_GTK_EXPORTMODELCOLUMNS_H
#define COLD_ARC_GTK_EXPORTMODELCOLUMNS_H

#include <gtkmm-3.0/gtkmm.h>

class ExportModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<gint> percentage;
        Gtk::TreeModelColumn<Glib::ustring> file_path;

        ExportModelColumns() {
            add(percentage);
            add(file_path);
        }
};


#endif //COLD_ARC_GTK_EXPORTMODELCOLUMNS_H
