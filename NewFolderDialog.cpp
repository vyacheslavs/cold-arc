//
// Created by developer on 8/7/23.
//

#include "NewFolderDialog.h"
#include "Utils.h"
#include "Archive.h"

NewFolderDialog::NewFolderDialog(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    edit_folder_name = findWidget<Gtk::Entry>("edit_folder_name", builder);
}

void NewFolderDialog::run() {
    runDialog<NewFolderDialog>("/main/newfolder.glade", "settings_win", [](NewFolderDialog* dlg, int rc){
        auto folder_name = dlg->edit_folder_name->get_text();
        if (rc == Gtk::RESPONSE_OK && !folder_name.empty()) {
            arc::Archive::instance().createFolder(folder_name);
        }
    });
}
