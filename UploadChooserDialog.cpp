//
// Created by developer on 8/8/23.
//

#include "UploadChooserDialog.h"
#include "Utils.h"

UploadChooserDialog::UploadChooserDialog(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    m_chooser_widget = findWidget<Gtk::FileChooserWidget>("chooser_widget", builder);
}

UploadFilesCollection UploadChooserDialog::run() {

    UploadFilesCollection files_to_upload;
    runDialog<UploadChooserDialog>("/main/uploaddialog.glade", "upload_dlg", [&](UploadChooserDialog* dlg, int rc){
        if (rc == Gtk::RESPONSE_OK)
            files_to_upload = std::move(dlg->getFiles());
    });

    return std::move(files_to_upload);
}

UploadFilesCollection UploadChooserDialog::getFiles() const {
    return std::move(m_chooser_widget->get_files());
}
