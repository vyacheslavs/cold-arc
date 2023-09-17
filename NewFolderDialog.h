//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_NEWFOLDERDIALOG_H
#define COLD_ARC_GTK_NEWFOLDERDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "Utils.h"

class NewFolderDialog : public Gtk::Dialog {

    public:
        NewFolderDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

        [[nodiscard]] static cold_arc::Result<> run(uint64_t parentId);
        cold_arc::Result<> construct(const Glib::RefPtr<Gtk::Builder>& builder);

    private:
        void onActivate();
        Gtk::Entry* edit_folder_name {nullptr};
};


#endif //COLD_ARC_GTK_NEWFOLDERDIALOG_H
