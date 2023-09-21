//
// Created by developer on 8/7/23.
//

#include "NewFolderDialog.h"
#include "Utils.h"
#include "Archive.h"

NewFolderDialog::NewFolderDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(win) {}

cold_arc::Result<> NewFolderDialog::run(uint64_t parentId) {
    auto r = runDialog<NewFolderDialog>("/main/newfolder.glade", "settings_win");
    if (!r)
        return unexpected_nested(cold_arc::ErrorCode::NewFolderDialogError, r.error());

    auto folder_name = r.value().dialog->edit_folder_name->get_text();
    if (r.value().rc == Gtk::RESPONSE_OK && !folder_name.empty()) {

        if (arc::Archive::instance().settings->is_paranoic()) {
            std::stringstream ss;
            ss << "Save point before new folder ("<<folder_name<<") creation";
            if (auto res = arc::Archive::instance().commit(ss.str()); !res)
                return unexpected_nested(cold_arc::ErrorCode::NewFolderDialogError, res.error());
        }

        if (auto res = arc::Archive::instance().beginTransaction();!res)
            return unexpected_nested(cold_arc::ErrorCode::NewFolderDialogError, res.error());

        if (auto res = arc::Archive::instance().createFolder(folder_name, parentId);!res) {
            auto rb = arc::Archive::instance().rollbackTransaction();
            return unexpected_combined_error(cold_arc::ErrorCode::NewFolderDialogError, res.error(), rb);
        }
        if (auto res = arc::Archive::instance().commitTransaction();!res)
            return unexpected_nested(cold_arc::ErrorCode::NewFolderDialogError, res.error());
    }
    return {};
}

void NewFolderDialog::onActivate() {
    response(Gtk::RESPONSE_OK);
}

cold_arc::Result<> NewFolderDialog::construct(const Glib::RefPtr<Gtk::Builder>& builder) {
    edit_folder_name = findWidget<Gtk::Entry>("edit_folder_name", builder);
    edit_folder_name->signal_activate().connect(sigc::mem_fun(this, &NewFolderDialog::onActivate));
    return {};
}
