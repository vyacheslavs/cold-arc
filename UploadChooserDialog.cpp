//
// Created by developer on 8/8/23.
//

#include "UploadChooserDialog.h"
#include "Utils.h"
#include "Signals.h"

UploadChooserDialog::UploadChooserDialog(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    m_chooser_widget = findWidget<Gtk::FileChooserWidget>("chooser_widget", builder);
}

void UploadChooserDialog::run() {

    std::vector<Glib::RefPtr<Gio::File> > files_to_upload;
    runDialog<UploadChooserDialog>("/main/uploaddialog.glade", "upload_dlg", [&](UploadChooserDialog* dlg, int rc){
        files_to_upload = std::move(dlg->getFiles());
    });

    Signals::instance().upload_files.emit(files_to_upload);

    /*for (auto const& f : files_to_upload) {
        std::cout << f->get_path() << " type: " << f->query_file_type(Gio::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS) << "\n";
    }*/
}

std::vector<Glib::RefPtr<Gio::File> > UploadChooserDialog::getFiles() const {
    return m_chooser_widget->get_files();
}
