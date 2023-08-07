//
// Created by developer on 8/5/23.
//

#include <iostream>
#include "MainWindow.h"
#include "Archive.h"
#include "ArchiveSettings.h"

MainWindow::MainWindow(Gtk::Window::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Window(win), m_builder(builder) {

    auto initButton = [&](const Glib::ustring& name, Gtk::ToolButton* &btn) {
        m_builder->get_widget<Gtk::ToolButton>(name, btn);
        if (!btn)
            throw std::runtime_error("failed to initialize ui");
    };

    initButton("upload_folder", m_upload_folder_button);
    initButton("new_archive_button", m_new_archive_button);
    initButton("open_archive_button", m_open_archive_button);
    initButton("settings_button", m_archive_settings_button);

    m_new_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewArchiveButtonClicked));
    m_open_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onOpenArchiveButtonClicked));
    m_archive_settings_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onArchiveSettings));
    setArchiveLoaded(false);
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

    if (newArcDlg.run() == Gtk::RESPONSE_OK) {
        arc::Archive::instance().newArchive(newArcDlg.get_filename());
        setArchiveLoaded(true);
    }
}

void MainWindow::onOpenArchiveButtonClicked() {
    Gtk::FileChooserDialog openArcDlg("Please choose the name of archive database", Gtk::FILE_CHOOSER_ACTION_SAVE);
    openArcDlg.set_transient_for(*this);

    openArcDlg.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    openArcDlg.add_button("Open", Gtk::RESPONSE_OK);

    auto dbFilter = Gtk::FileFilter::create();
    dbFilter->set_name("sqlite files");
    dbFilter->add_mime_type("application/vnd.sqlite3");
    dbFilter->add_mime_type("application/x-sqlite3");
    openArcDlg.add_filter(dbFilter);

    switch (openArcDlg.run()) {
        case Gtk::RESPONSE_OK: {
            // set_title(Glib::ustring::compose("ColdArc [%1]",arc::Archive::instance().settings->name()));
            break;
        }
        default:
            break;
    }
}

void MainWindow::setArchiveLoaded(bool loaded) {
    m_upload_folder_button->set_visible(loaded);
    m_archive_settings_button->set_visible(loaded);
    if (loaded) {
        set_title(Glib::ustring::compose("ColdArc [%1]", arc::Archive::instance().settings->name()));
    } else {
        set_title("ColdArc");
    }
}

void MainWindow::onArchiveSettings() {
    std::unique_ptr<ArchiveSettings> dlg(ArchiveSettings::create(m_builder));
    dlg->run();
    // m_app->run(*dlg);
}
