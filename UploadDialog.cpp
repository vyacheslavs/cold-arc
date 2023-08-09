//
// Created by developer on 8/9/23.
//

#include "UploadDialog.h"
#include "Utils.h"

UploadDialog::UploadDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder,
                           UploadFilesCollection&& files) : Gtk::Dialog(win), m_files(std::move(files)) {
}

void UploadDialog::run(UploadFilesCollection&& files) {
    runDialog<UploadDialog>("/main/updlg.glade", "upload_dialog", [&](UploadDialog* dlg, int rc) {
    }, files);
}
