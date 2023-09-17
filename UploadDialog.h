//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADDIALOG_H
#define COLD_ARC_GTK_UPLOADDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "UploadFilesCollection.h"
#include "UploadStage1Resolving.h"
#include "UploadStage2DbUpdate.h"
#include "Utils.h"

class UploadDialog : public Gtk::Dialog {
public:
    UploadDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

    [[nodiscard]] static cold_arc::Result<bool> run(uint64_t current_folder_parent_id, UploadFilesCollection&& files);
    [[nodiscard]] cold_arc::Result<> construct(const Glib::RefPtr<Gtk::Builder>& builder, uint64_t current_folder_parent_id, UploadFilesCollection&& files);

private:

    void onStage1Notification(const UploadFileInfo& notification);
    void onStage2Update(uint64_t id, uint64_t total, bool shut, const cold_arc::Error& e);
    void onRemoveButtonClicked();
    void onRemoveErrButtonClicked();
    void onNextButtonClicked();

    std::unique_ptr<UploadStage1Resolving> m_stage1;
    std::unique_ptr<UploadStage2DbUpdate> m_stage2;
    Gtk::TreeView* m_tree {nullptr};
    Gtk::ListStore* m_store {nullptr};
    Gtk::ProgressBar* m_progress {nullptr};
    Gtk::ToolButton* m_remove_button {nullptr};
    Gtk::ToolButton* m_remove_all_skipped {nullptr};
    Gtk::Button* m_btn_next {nullptr};
    Gtk::Button* m_btn_close {nullptr};
    std::vector<UploadFileInfo> m_ready_files;
    uint64_t m_current_folder_parent_id {0};
};


#endif //COLD_ARC_GTK_UPLOADDIALOG_H
