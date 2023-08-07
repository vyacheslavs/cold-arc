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
    Gtk::ToolButton* m_upload_folder_button;
    Gtk::ToolButton* m_open_archive_button;
    Gtk::ToolButton* m_new_archive_button;
    Gtk::ToolButton* m_archive_settings_button;
    Gtk::ToolButton* m_add_new_media_button;

    void onNewArchiveButtonClicked();
    void onOpenArchiveButtonClicked();
    void onArchiveSettings();
    void setArchiveLoaded(bool loaded);
};


#endif //COLD_ARC_GTK_MAINWINDOW_H
