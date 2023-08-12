//
// Created by developer on 8/5/23.
//

#ifndef COLD_ARC_GTK_MAINWINDOW_H
#define COLD_ARC_GTK_MAINWINDOW_H

#include <gtkmm-3.0/gtkmm.h>
#include "FolderModelColumns.h"

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
        Gtk::TreeView* m_tree;
        Gtk::TreeSelection* m_selection;
        Gtk::TreeStore* m_tree_store;
        Gtk::TreeView* m_contents_view;
        Gtk::ListStore* m_contents_store;
        Gtk::TreeView* m_media_view;
        Gtk::ListStore* m_media_store;
        Gtk::SeparatorToolItem* m_sep1;
        Gtk::SeparatorToolItem* m_sep2;
        std::unordered_map<uint64_t, Gtk::TreeIter> m_tree_fast_access;
        Gtk::Toolbar* m_media_toolbar;
        Gtk::ToolButton* m_media_new_button;

        void onNewArchiveButtonClicked();

        void onOpenArchiveButtonClicked();

        void onArchiveSettings();

        void onNewMediaButtonClicked();

        void onCreateFolderClicked();

        void updateUI();

        void updateTree();

        void updateContents();

        void updateMediaView();

        void onUploadButtonClicked();

        void onMediaToggle(const Glib::ustring& path);

        uint64_t currentFolderParentId();

        template<typename A, typename B, typename C>
        void allocateTreeNode(A it, B id, C name) {
            m_tree_fast_access[id] = it;
            const auto& row = *it;

            FolderModelColumns cols;
            row[cols.folder] = name;
            row[cols.id] = id;
            row[cols.status] = Gdk::Pixbuf::create_from_resource(id == 1 ? "/icons/ca-root.svg" : "/icons/ca-folder.svg");
        }

        void allocateTreeNodeUsingParentId(const Glib::ustring& folderName, uint64_t id, uint64_t parentId);

        std::string collectExclusions();
};


#endif //COLD_ARC_GTK_MAINWINDOW_H
