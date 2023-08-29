//
// Created by developer on 8/7/23.
//

#include "NewFolderDialog.h"
#include "Utils.h"
#include "Archive.h"

NewFolderDialog::NewFolderDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(win) {
    edit_folder_name = findWidget<Gtk::Entry>("edit_folder_name", builder);
    edit_folder_name->signal_activate().connect(sigc::mem_fun(this, &NewFolderDialog::onActivate));
}

void NewFolderDialog::run(uint64_t parentId) {
    runDialog<NewFolderDialog>("/main/newfolder.glade", "settings_win", [&](NewFolderDialog* dlg, int rc) {
        auto folder_name = dlg->edit_folder_name->get_text();
        if (rc == Gtk::RESPONSE_OK && !folder_name.empty()) {
            auto p = arc::Archive::instance().savePoint();
            try {
                arc::Archive::instance().createFolder(folder_name, parentId);
            } catch (const sqlite::sqlite_exception& e) {
                p.rollback();
                sqliteError(e, false);
            }
        }
    });
}

void NewFolderDialog::onActivate() {
    response(Gtk::RESPONSE_OK);
}
