//
// Created by developer on 8/8/23.
//

#include "UploadChooserDialog.h"
#include "Utils.h"

UploadChooserDialog::UploadChooserDialog(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    m_chooser_widget = findWidget<Gtk::FileChooserWidget>("chooser_widget", builder);
}

cold_arc::Result<UploadFilesCollection> UploadChooserDialog::run() {
    auto r = runDialog<UploadChooserDialog>("/main/uploaddialog.glade", "upload_dlg");
    if (!r)
        return unexpected_nested(cold_arc::ErrorCode::UploadChooserDialogError, r.error());

    if (r.value().rc == Gtk::RESPONSE_OK)
        return std::move(r.value().dialog->getFiles());

    return {};
}

UploadFilesCollection UploadChooserDialog::getFiles() const {
    return std::move(m_chooser_widget->get_files());
}
