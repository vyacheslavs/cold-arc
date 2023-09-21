//
// Created by developer on 9/20/23.
//

#ifndef COLD_ARC_GTK_HISTORYMODELCOLUMNS_H
#define COLD_ARC_GTK_HISTORYMODELCOLUMNS_H

#include <gtkmm-3.0/gtkmm.h>

class HistoryModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<gboolean> cursor;
        Gtk::TreeModelColumn<guint64> id;
        Gtk::TreeModelColumn<guint64> date;
        Gtk::TreeModelColumn<Glib::ustring> description;
        Gtk::TreeModelColumn<Glib::ustring> date_string;

        HistoryModelColumns() {
            add(cursor);
            add(id);
            add(date);
            add(description);
            add(date_string);
        }
};


#endif //COLD_ARC_GTK_HISTORYMODELCOLUMNS_H
