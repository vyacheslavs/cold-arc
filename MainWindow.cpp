//
// Created by developer on 8/5/23.
//

#include <iostream>
#include "MainWindow.h"
#include "Archive.h"
#include "ArchiveSettingsDialog.h"
#include "Utils.h"
#include "NewMediaDialog.h"
#include "Signals.h"
#include "NewFolderDialog.h"
#include "FolderModelColumns.h"

MainWindow::MainWindow(Gtk::Window::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Window(win), m_builder(builder) {

    m_upload_button = findWidget<Gtk::ToolButton>("upload_btn", m_builder);
    m_create_folder = findWidget<Gtk::ToolButton>("create_folder_btn", m_builder);
    m_new_archive_button = findWidget<Gtk::ToolButton>("new_archive_button", m_builder);
    m_open_archive_button = findWidget<Gtk::ToolButton>("open_archive_button", m_builder);
    m_archive_settings_button = findWidget<Gtk::ToolButton>("settings_button", m_builder);
    m_add_new_media_button = findWidget<Gtk::ToolButton>("new_media_btn", m_builder);
    m_tree = findWidget<Gtk::TreeView>("treeview", m_builder);

    m_new_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewArchiveButtonClicked));
    m_open_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onOpenArchiveButtonClicked));
    m_archive_settings_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onArchiveSettings));
    m_add_new_media_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewMediaButtonClicked));
    m_create_folder->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onCreateFolderClicked));

    Signals::instance().update_main_window.connect(sigc::mem_fun(this, &MainWindow::updateUI));
    Signals::instance().new_folder.connect(sigc::mem_fun(this, &MainWindow::onNewFolder));
    Signals::instance().update_tree.connect(sigc::mem_fun(this, &MainWindow::updateTree));

    FolderModelColumns cols;
    m_tree->append_column("Folder", cols.folder);
    // Gdk::RGBA c;
    //m_tree->get_style_context()->get_color(Gtk::STATE_FLAG_PRELIGHT);
    // m_tree->override_background_color(m_tree->get_style_context()->get_color(Gtk::STATE_FLAG_VISITED));
    updateUI();
    updateTree();
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

    if (openArcDlg.run() == Gtk::RESPONSE_OK) {
        arc::Archive::instance().openArchive(openArcDlg.get_filename());
    }
}

void MainWindow::updateUI() {
    m_upload_button->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_create_folder->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_archive_settings_button->set_visible(arc::Archive::instance().hasActiveArchive());
    m_add_new_media_button->set_visible(arc::Archive::instance().hasActiveArchive());
    auto title = Glib::ustring("ColdArc");
    if (arc::Archive::instance().hasActiveArchive()) {
        title = Glib::ustring::compose("ColdArc [%1]", arc::Archive::instance().settings->name());
        if (arc::Archive::instance().hasCurrentMedia() && arc::Archive::instance().settings->media()) {
            title += Glib::ustring::compose(" %1 / %2", arc::Archive::instance().settings->media()->name(),arc::Archive::instance().settings->media()->serial());
        }
    }
    set_title(title);
}

void MainWindow::onArchiveSettings() {
    ArchiveSettingsDialog::run();
}

void MainWindow::onNewMediaButtonClicked() {
    NewMediaDialog::run();
}

void MainWindow::onCreateFolderClicked() {
    uint64_t parentId = 0;
    Glib::ustring val;

    if (m_tree->get_selection()->get_selected()) {
        auto row = *(m_tree->get_selection()->get_selected());
        FolderModelColumns cols;
        parentId = row[cols.id];
    }

    NewFolderDialog::run(parentId);
}

void MainWindow::onNewFolder(const Glib::ustring &folderName, uint64_t id, uint64_t parentId) {

    auto items = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic(
            m_builder->get_object("treestore1")
    );

    // Gtk::TreeIter it = items->append();

    auto row_it = [&]() {
        if (parentId == 0)
            return items->append();
        else {
            return items->append(m_tree->get_selection()->get_selected()->children());
        }
    }();

    m_tree_fast_access[id] = row_it;
    auto row = *row_it;

    FolderModelColumns cols;
    row[cols.folder] = folderName;
    row[cols.id] = id;
}

void MainWindow::updateTree() {

    auto items = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic(m_builder->get_object("treestore1"));
    items->clear();

    auto allocate = [&](auto it, auto id, auto name) {
        m_tree_fast_access[id] = it;
        auto row = *it;

        FolderModelColumns cols;
        row[cols.folder] = name;
        row[cols.id] = id;
    };

    arc::Archive::instance().walkTree([&](sqlite3_uint64 id, const char* typ, const char* name, const char* hash, const char* lnk, sqlite3_uint64 dt, sqlite3_uint64 parent_id) {
        if (parent_id == 0) {
            allocate(items->append(), id, name);
        } else {
            auto t_it = m_tree_fast_access.find(parent_id);
            if (t_it != m_tree_fast_access.end()) {
                allocate(items->append(t_it->second->children()), id, name);
            }
        }
    });
}
