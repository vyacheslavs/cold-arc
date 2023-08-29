//
// Created by developer on 8/9/23.
//

#include "UploadDialog.h"
#include "Utils.h"
#include "UploadListColumns.h"
#include "Archive.h"

#include <iostream>

UploadDialog::UploadDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder,
    uint64_t current_folder_parent_id, UploadFilesCollection&& files) : Gtk::Dialog(win),
                                                                        m_stage1(std::move(
                                                                            files)),
                                                                        m_current_folder_parent_id(
                                                                            current_folder_parent_id) {

    m_stage1.signal_upload_notification(sigc::mem_fun(this, &UploadDialog::onStage1Notification));

    m_tree = findWidget<Gtk::TreeView>("tree", builder);
    m_progress = findWidget<Gtk::ProgressBar>("progress", builder);
    m_remove_button = findWidget<Gtk::ToolButton>("btn_remove", builder);
    m_remove_all_skipped = findWidget<Gtk::ToolButton>("btn_remove_err", builder);
    m_btn_next = findWidget<Gtk::Button>("btn_next", builder);
    m_btn_close = findWidget<Gtk::Button>("btn_close", builder);

    m_remove_button->signal_clicked().connect(sigc::mem_fun(this, &UploadDialog::onRemoveButtonClicked));
    m_remove_all_skipped->signal_clicked().connect(sigc::mem_fun(this, &UploadDialog::onRemoveErrButtonClicked));
    m_btn_next->signal_clicked().connect(sigc::mem_fun(this, &UploadDialog::onNextButtonClicked));
    m_btn_next->set_sensitive(false);
    m_btn_close->set_sensitive(false);
    m_remove_button->set_sensitive(false);
    m_remove_all_skipped->set_sensitive(false);

    UploadListColumns cols;
    m_tree->append_column("Status", cols.status);
    m_tree->append_column("Folder", cols.folder);
    m_tree->append_column("Path", cols.path);
    m_tree->append_column("SHA256", cols.hash);
    m_tree->append_column("Size", cols.size);
    m_tree->append_column("Info", cols.reason);

    m_store = findObject<Gtk::ListStore>("resolve_tree_list_store", builder);
}

bool UploadDialog::run(uint64_t current_folder_parent_id, UploadFilesCollection&& files) {
    bool ret = false;
    runDialog<UploadDialog>("/main/updlg.glade", "upload_dialog", [&](UploadDialog* dlg, int rc) {
        ret = rc == Gtk::RESPONSE_OK;
    }, current_folder_parent_id, files);
    return ret;
}

void UploadDialog::onStage1Notification(const UploadFileInfo& notification) {
    if (notification.isProcessed()) {
        auto row = *m_store->append();
        UploadListColumns cols;
        m_ready_files.push_back(notification);
        row[cols.status] = Gdk::Pixbuf::create_from_resource("/icons/ca-check.svg");
        row[cols.path] = notification.getBasename();
        row[cols.reason] = "OK";
        m_ready_files.rbegin()->index = m_ready_files.size();
        row[cols.data] = m_ready_files.size();
        row[cols.folder] = notification.getFolder().empty() ? "/" : notification.getFolder();
        row[cols.hash] = notification.getHash();
        row[cols.size] = notification.getSize();
        m_progress->set_fraction(notification.fraction());
    } else if (notification.isSkipped()) {
        auto row = *m_store->insert(m_store->children().begin());
        UploadListColumns cols;
        row[cols.status] = Gdk::Pixbuf::create_from_resource("/icons/ca-skipped.svg");
        row[cols.path] = notification.getPath();
        row[cols.reason] = "skipped";
        row[cols.data] = 0;
    } else if (notification.isFailedToOpen()) {
        auto row = *m_store->insert(m_store->children().begin());
        UploadListColumns cols;
        row[cols.status] = Gdk::Pixbuf::create_from_resource("/icons/ca-error.svg");
        row[cols.path] = notification.getPath();
        row[cols.reason] = "failed to open";
        row[cols.data] = 0;
    } else if (notification.isFailedToHash()) {
        auto row = *m_store->insert(m_store->children().begin());
        UploadListColumns cols;
        row[cols.status] = Gdk::Pixbuf::create_from_resource("/icons/ca-error.svg");
        row[cols.path] = notification.getPath();
        row[cols.reason] = "failed to hash";
        row[cols.data] = 0;
    } else if (notification.isHashing()) {
        auto percent = static_cast<int>(notification.fraction() * 100);
        if (percent>0)
            m_progress->set_text(Glib::ustring::compose("Hashing %1 (%2%%) ...", notification.getBasename(),
                                                    percent));
        else
            m_progress->set_text(Glib::ustring::compose("Hashing %1 ...", notification.getBasename()));
    } else if (notification.isThreadStopped()) {
        m_progress->set_fraction(1);
        m_progress->set_text("Complete");
        m_btn_close->set_sensitive(true);
        m_btn_next->set_sensitive(!m_store->children().empty());
        m_remove_button->set_sensitive(true);
        m_remove_all_skipped->set_sensitive(true);
    }
}

void UploadDialog::onRemoveButtonClicked() {
    UploadListColumns cols;
    auto selected = m_tree->get_selection()->get_selected();
    if (selected) {
        auto index = (*selected)[cols.data];
        if (index > 0)
            m_ready_files[index - 1].skip();
        m_store->erase(selected);
        m_btn_next->set_sensitive(!m_store->children().empty());
    }
}

void UploadDialog::onRemoveErrButtonClicked() {
    UploadListColumns cols;
    for (auto it = m_store->children().begin(); it != m_store->children().end();) {
        auto index = (*it)[cols.data];
        if (index == 0) {
            it = m_store->erase(it);
        } else {
            ++it;
        }
    }
    m_btn_next->set_sensitive(!m_store->children().empty());
}

void UploadDialog::onNextButtonClicked() {

    // before anything let's check the media
    uint64_t total = 0;
    for (const auto& item: m_ready_files) {
        if (item.isSkipped())
            continue;
        total += item.getSize();
    }

    if (arc::Archive::instance().settings->media()->free() < total) {
        // not enough place to allocate all those files
        Gtk::MessageDialog rusure("Not enough space to allocate the files.", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL,
                                  true);
        rusure.set_secondary_text("You can stop uploading or upload files partially until full");
        if (rusure.run() == Gtk::RESPONSE_CANCEL) {
            response(Gtk::RESPONSE_CANCEL);
            return;
        }
    }

    onRemoveErrButtonClicked();

    m_btn_next->set_sensitive(false);
    m_btn_close->set_sensitive(false);
    m_remove_button->set_sensitive(false);
    m_remove_all_skipped->set_sensitive(false);

    m_stage2 = std::make_unique<UploadStage2DbUpdate>(std::move(m_ready_files), m_current_folder_parent_id);
    m_stage2->signal_update_notification(sigc::mem_fun(this, &UploadDialog::onStage2Update));
    m_stage2->start(arc::Archive::instance().settings->media()->free());
}

void UploadDialog::onStage2Update(uint64_t id, uint64_t total, bool shut, std::shared_ptr<ExceptionCargoBase> _e) {

    if (_e) {
        try {
            response(Gtk::RESPONSE_CANCEL);
            _e->rethrow();
        } catch (const sqlite::sqlite_exception& e) {
            response(Gtk::RESPONSE_CANCEL);
            sqliteError(e);
            return;
        } catch (const WrongDatabaseVersion& e) {
            response(Gtk::RESPONSE_CANCEL);
            Gtk::MessageDialog dlg("Failed to upload data");
            dlg.set_secondary_text(e.what());
            dlg.run();
            return;
        }
    }

    if (id>0) {
        UploadListColumns cols;
        for (auto it = m_store->children().begin(); it != m_store->children().end();) {
            auto index = (*it)[cols.data];
            if (index < id)
                it = m_store->erase(it);
            else
                break;
        }
        m_progress->set_fraction(static_cast<double>(id) / static_cast<double>(total));
        m_progress->set_text(Glib::ustring());
    }
    if (shut) {
        m_store->clear();
        m_progress->set_fraction(1);
        m_progress->set_text("Upload complete");
        m_btn_close->set_sensitive(true);
        response(Gtk::RESPONSE_OK);
    }
}

