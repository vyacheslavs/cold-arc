//
// Created by developer on 8/5/23.
//

#include <iostream>
#include "MainWindow.h"
#include "Archive.h"

MainWindow::MainWindow(Gtk::Window::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Window(win), m_builder(builder) {

    auto initButton = [&](const Glib::ustring& name, Gtk::ToolButton* &btn) {
        m_builder->get_widget<Gtk::ToolButton>(name, btn);
        if (!btn)
            throw std::runtime_error("failed to initialize ui");
    };

    initButton("upload_folder", m_upload_folder_button);
    initButton("new_archive_button", m_new_archive_button);
    initButton("open_archive_button", m_open_archive_button);

    m_new_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewArchiveButtonClicked));
    m_upload_folder_button->set_sensitive(false);
}

void MainWindow::onNewArchiveButtonClicked() {
    Gtk::FileChooserDialog newArcDlg("Please choose the name of archive database", Gtk::FILE_CHOOSER_ACTION_SAVE);
    newArcDlg.set_transient_for(*this);

    newArcDlg.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    newArcDlg.add_button("Save", Gtk::RESPONSE_OK);

    auto dbFilter = Gtk::FileFilter::create();
    dbFilter->set_name("sqlite files");
    dbFilter->add_mime_type("application/vnd.sqlite3");
    dbFilter->add_mime_type("application/x-sqlite3");
    newArcDlg.add_filter(dbFilter);

    switch (newArcDlg.run()) {
        case Gtk::RESPONSE_OK: {
            arc::Archive::instance().newArchive(newArcDlg.get_filename());
            break;
        }
        default:
            break;
    }
}
