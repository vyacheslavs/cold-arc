//
// Created by developer on 8/5/23.
//

#ifndef COLD_ARC_GTK_MAINWINDOW_H
#define COLD_ARC_GTK_MAINWINDOW_H

#include <gtkmm-3.0/gtkmm.h>

class MainWindow : public Gtk::Window {
public:
    MainWindow(Gtk::Window::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

private:

    const Glib::RefPtr<Gtk::Builder>& m_builder;
    Gtk::ToolButton* m_upload_button;
    Gtk::ToolButton* m_create_folder;
    Gtk::ToolButton* m_open_archive_button;
    Gtk::ToolButton* m_new_archive_button;
    Gtk::ToolButton* m_archive_settings_button;
    Gtk::ToolButton* m_add_new_media_button;
    Gtk::TreeView* m_tree;

    void onNewArchiveButtonClicked();
    void onOpenArchiveButtonClicked();
    void onArchiveSettings();
    void onNewMediaButtonClicked();
    void onCreateFolderClicked();
    void updateUI();
    void onNewFolder(const Glib::ustring &folderName, uint64_t id, uint64_t parentId);
};


#endif //COLD_ARC_GTK_MAINWINDOW_H
