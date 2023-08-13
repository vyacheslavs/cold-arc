//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADDIALOG_H
#define COLD_ARC_GTK_UPLOADDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "UploadFilesCollection.h"
#include "UploadStage1Resolving.h"
#include "UploadStage2DbUpdate.h"

class UploadDialog : public Gtk::Dialog {
public:
    UploadDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder,
                 uint64_t current_folder_parent_id, UploadFilesCollection&& files);

    static bool run(uint64_t current_folder_parent_id, UploadFilesCollection&& files);

private:

    void onStage1Notification(const UploadFileInfo& notification);
    void onStage2Update(uint64_t id, uint64_t total, bool shut);
    void onRemoveButtonClicked();
    void onRemoveErrButtonClicked();
    void onNextButtonClicked();

    UploadStage1Resolving m_stage1;
    std::unique_ptr<UploadStage2DbUpdate> m_stage2;
    Gtk::TreeView* m_tree;
    Gtk::ListStore* m_store;
    Gtk::ProgressBar* m_progress;
    Gtk::ToolButton* m_remove_button;
    Gtk::ToolButton* m_remove_all_skipped;
    Gtk::Button* m_btn_next;
    Gtk::Button* m_btn_close;
    std::vector<UploadFileInfo> m_ready_files;
    uint64_t m_current_folder_parent_id;
};


#endif //COLD_ARC_GTK_UPLOADDIALOG_H
