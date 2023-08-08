//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_NEWFOLDERDIALOG_H
#define COLD_ARC_GTK_NEWFOLDERDIALOG_H

#include <gtkmm-3.0/gtkmm.h>

class NewFolderDialog : public Gtk::Dialog {

public:
    NewFolderDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);
    static void run(uint64_t parentId);

    Gtk::Entry* edit_folder_name;
};


#endif //COLD_ARC_GTK_NEWFOLDERDIALOG_H
