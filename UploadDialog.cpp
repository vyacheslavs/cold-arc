//
// Created by developer on 8/9/23.
//

#include "UploadDialog.h"
#include "Utils.h"
#include "UploadListColumns.h"

#include <iostream>

UploadDialog::UploadDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder,
                           UploadFilesCollection&& files) : Gtk::Dialog(win), m_stage1(std::move(files)) {

    m_stage1.signal_upload_notification(sigc::mem_fun(this, &UploadDialog::onStage1Notification));

    m_tree = findWidget<Gtk::TreeView>("tree", builder);
    m_progress = findWidget<Gtk::ProgressBar>("progress", builder);

    UploadListColumns cols;
    m_tree->append_column("Status", cols.status);
    m_tree->append_column("Path", cols.path);
    m_tree->append_column("Info", cols.reason);

    m_store = findObject<Gtk::ListStore>("resolve_tree_list_store", builder);
}

void UploadDialog::run(UploadFilesCollection&& files) {
    runDialog<UploadDialog>("/main/updlg.glade", "upload_dialog", [&](UploadDialog* dlg, int rc) {
    }, files);
}

void UploadDialog::onStage1Notification(const UploadStage1Notification& notification) {
    if (notification.isProcessed()) {
        auto row = *m_store->append();
        UploadListColumns cols;
        row[cols.status] = Gdk::Pixbuf::create_from_resource("/icons/ca-check.svg");
        row[cols.path] = notification.getBasename();
        row[cols.reason] = "OK";
        m_progress->set_fraction(notification.fraction());
    } else if (notification.isSkipped()) {
        // auto row = *m_store->append();
        auto row = *m_store->insert(m_store->children().begin());
        UploadListColumns cols;
        row[cols.status] = Gdk::Pixbuf::create_from_resource("/icons/ca-skipped.svg");
        row[cols.path] = notification.getPath();
        row[cols.reason] = "skipped";
    } else if (notification.isHashing()) {
        m_progress->set_text(Glib::ustring::compose("Hashing %1 .. %2", notification.getPath(),
                                                    static_cast<int>(notification.fraction() * 100)));
    } else if (notification.isThreadStopped()) {
        m_progress->set_fraction(1);
        m_progress->set_text("Complete");
    }
}

