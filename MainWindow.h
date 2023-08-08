//
// Created by developer on 8/5/23.
//

#ifndef COLD_ARC_GTK_MAINWINDOW_H
#define COLD_ARC_GTK_MAINWINDOW_H

#include <gtkmm-3.0/gtkmm.h>
#include "FolderModelColumns.h"
#include "ProgressWindow.h"
#include "ProgressInfo.h"

class MainWindow : public Gtk::Window {
public:
    MainWindow(Gtk::Window::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

private:

    const Glib::RefPtr<Gtk::Builder> m_builder;
    Gtk::ToolButton* m_upload_button;
    Gtk::ToolButton* m_create_folder;
    Gtk::ToolButton* m_open_archive_button;
    Gtk::ToolButton* m_new_archive_button;
    Gtk::ToolButton* m_archive_settings_button;
    Gtk::ToolButton* m_add_new_media_button;
    Gtk::ToolButton* m_show_progress_button;
    Gtk::TreeView* m_tree;
    std::unordered_map<uint64_t, Gtk::TreeIter> m_tree_fast_access;
    std::unique_ptr<ProgressWindow> m_progress_window;

    void onNewArchiveButtonClicked();
    void onOpenArchiveButtonClicked();
    void onArchiveSettings();
    void onNewMediaButtonClicked();
    void onCreateFolderClicked();
    void updateUI();
    void updateTree();
    void onUploadButtonClicked();
    void onUploadProgress(const ProgressInfo&);

    template <typename A, typename B, typename C>
    void allocateTreeNode(A it, B id, C name) {
        m_tree_fast_access[id] = it;
        const auto& row = *it;

        FolderModelColumns cols;
        row[cols.folder] = name;
        row[cols.id] = id;
    }

    void allocateTreeNodeUsingParentId(const Glib::ustring &folderName, uint64_t id, uint64_t parentId);
};


#endif //COLD_ARC_GTK_MAINWINDOW_H
