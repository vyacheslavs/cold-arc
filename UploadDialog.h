//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADDIALOG_H
#define COLD_ARC_GTK_UPLOADDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "UploadFilesCollection.h"
#include "UploadStage1Resolving.h"

class UploadDialog : public Gtk::Dialog {
public:
    UploadDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder,
                 UploadFilesCollection&& files);

    static void run(UploadFilesCollection&& files);

private:

    void onStage1Notification(const UploadStage1Notification& notification);
    void onRemoveButtonClicked();
    void onRemoveErrButtonClicked();
    void onNextButtonClicked();

    UploadStage1Resolving m_stage1;
    Gtk::TreeView* m_tree;
    Gtk::ListStore* m_store;
    Gtk::ProgressBar* m_progress;
    Gtk::ToolButton* m_remove_button;
    Gtk::ToolButton* m_remove_all_skipped;
    Gtk::Button* m_btn_next;
    Gtk::Button* m_btn_close;
    std::vector<UploadStage1Notification> m_ready_files;
};


#endif //COLD_ARC_GTK_UPLOADDIALOG_H
