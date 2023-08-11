//
// Created by developer on 8/5/23.
//

#include "MainWindow.h"
#include "Archive.h"
#include "ArchiveSettingsDialog.h"
#include "Utils.h"
#include "NewMediaDialog.h"
#include "Signals.h"
#include "NewFolderDialog.h"
#include "FolderModelColumns.h"
#include "UploadChooserDialog.h"
#include "UploadDialog.h"
#include "ContentsModelColumns.h"
#include "MediaListColumns.h"

MainWindow::MainWindow(Gtk::Window::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Window(win),
                                                                                                      m_builder(
                                                                                                          builder) {

    auto applyFontAwesome = [&](auto widget) {
        auto desc = widget->get_pango_context()->get_font_description();
        desc.set_family("Font Awesome 6 Free");
        desc.set_size(18 * Pango::SCALE);
        desc.set_weight(Pango::WEIGHT_HEAVY);
        widget->get_pango_context()->set_font_description(desc);
    };

    m_upload_button = findWidget<Gtk::ToolButton>("upload_btn", m_builder);
    m_create_folder = findWidget<Gtk::ToolButton>("create_folder_btn", m_builder);
    m_new_archive_button = findWidget<Gtk::ToolButton>("new_archive_button", m_builder);
    m_open_archive_button = findWidget<Gtk::ToolButton>("open_archive_button", m_builder);
    m_archive_settings_button = findWidget<Gtk::ToolButton>("settings_button", m_builder);
    m_add_new_media_button = findWidget<Gtk::ToolButton>("new_media_btn", m_builder);
    m_tree = findWidget<Gtk::TreeView>("treeview", m_builder);
    m_selection = findObject<Gtk::TreeSelection>("selection", m_builder);
    m_tree_store = findObject<Gtk::TreeStore>("treestore1", m_builder);
    m_contents_view = findWidget<Gtk::TreeView>("contents", m_builder);
    m_contents_store = findObject<Gtk::ListStore>("liststore1", m_builder);
    m_media_view = findWidget<Gtk::TreeView>("mediaview", m_builder);
    m_media_store = findObject<Gtk::ListStore>("liststore2", m_builder);

    applyFontAwesome(m_open_archive_button->get_label_widget());
    applyFontAwesome(m_new_archive_button->get_label_widget());
    applyFontAwesome(m_add_new_media_button->get_label_widget());
    applyFontAwesome(m_archive_settings_button->get_label_widget());
    applyFontAwesome(m_create_folder->get_label_widget());
    applyFontAwesome(m_upload_button->get_label_widget());

    m_new_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewArchiveButtonClicked));
    m_open_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onOpenArchiveButtonClicked));
    m_archive_settings_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onArchiveSettings));
    m_add_new_media_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewMediaButtonClicked));
    m_create_folder->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onCreateFolderClicked));
    m_upload_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onUploadButtonClicked));
    m_selection->signal_changed().connect(sigc::mem_fun(this, &MainWindow::updateContents));

    Signals::instance().update_main_window.connect(sigc::mem_fun(this, &MainWindow::updateUI));
    Signals::instance().new_folder.connect(sigc::mem_fun(this, &MainWindow::allocateTreeNodeUsingParentId));
    Signals::instance().update_tree.connect(sigc::mem_fun(this, &MainWindow::updateTree));
    Signals::instance().update_media_view.connect(sigc::mem_fun(this, &MainWindow::updateMediaView));

    { // init treeview
        auto textRenderer = Gtk::manage(new Gtk::CellRendererText);
        auto iconRenderer = Gtk::manage(new Gtk::CellRendererPixbuf);
        Gtk::TreeView::Column* pColumn = Gtk::manage(new Gtk::TreeView::Column("Folder"));
        pColumn->pack_start(*iconRenderer, false);
        pColumn->pack_start(*textRenderer, true);

        m_tree->append_column(*pColumn);

        FolderModelColumns cols;
        pColumn->add_attribute(textRenderer->property_text(), cols.folder);
        pColumn->add_attribute(iconRenderer->property_pixbuf(), cols.status);
    }

    { // init contents
        ContentsModelColumns cols;

        auto textRenderer = Gtk::manage(new Gtk::CellRendererText);
        auto iconRenderer = Gtk::manage(new Gtk::CellRendererPixbuf);
        Gtk::TreeView::Column* pColumn = Gtk::manage(new Gtk::TreeView::Column("Name"));
        pColumn->pack_start(*iconRenderer, false);
        pColumn->pack_start(*textRenderer, true);
        m_contents_view->append_column(*pColumn);
        pColumn->add_attribute(textRenderer->property_text(), cols.name);
        pColumn->add_attribute(iconRenderer->property_pixbuf(), cols.typ);
        pColumn->set_resizable();

        m_contents_view->append_column("Size", cols.size);
        m_contents_view->append_column("Hash", cols.hash);
    }

    { // init media view
        MediaListColumns cols;
        m_media_view->append_column("", cols.active_icon);
        m_media_view->append_column("Name", cols.name);
    }

    updateUI();
    updateTree();
    updateMediaView();
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
    Gtk::FileChooserDialog openArcDlg("Please choose the name of archive database", Gtk::FILE_CHOOSER_ACTION_OPEN);
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

    auto theme = Gtk::IconTheme::get_default();
    theme->add_resource_path("/icons/app");

    m_upload_button->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_create_folder->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_archive_settings_button->set_visible(arc::Archive::instance().hasActiveArchive());
    m_add_new_media_button->set_visible(arc::Archive::instance().hasActiveArchive());
    auto title = Glib::ustring("ColdArc");
    if (arc::Archive::instance().hasActiveArchive()) {
        title = Glib::ustring::compose("ColdArc [%1]", arc::Archive::instance().settings->name());
        if (arc::Archive::instance().hasCurrentMedia() && arc::Archive::instance().settings->media()) {
            title += Glib::ustring::compose(" %1 / %2", arc::Archive::instance().settings->media()->name(),
                                            arc::Archive::instance().settings->media()->serial());
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
    NewFolderDialog::run(currentFolderParentId());
}

void MainWindow::updateTree() {

    if (!arc::Archive::instance().hasActiveArchive())
        return;

    m_tree_store->clear();
    m_contents_store->clear();

    arc::Archive::instance().walkTree(
        [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk,
            sqlite3_uint64 dt, sqlite3_uint64 parent_id) {
            allocateTreeNodeUsingParentId(name, id, parent_id);
        }, true);
}

void MainWindow::allocateTreeNodeUsingParentId(const Glib::ustring& name, uint64_t id, uint64_t parent_id) {
    if (parent_id == 0) {
        allocateTreeNode(m_tree_store->append(), id, name);
    } else {
        auto t_it = m_tree_fast_access.find(parent_id);
        if (t_it != m_tree_fast_access.end()) {
            allocateTreeNode(m_tree_store->append(t_it->second->children()), id, name);
        }
    }
}

void MainWindow::onUploadButtonClicked() {
    auto files = UploadChooserDialog::run();
    if (!files.empty() && UploadDialog::run(currentFolderParentId(), std::move(files))) {
        Signals::instance().update_tree.emit();
    }
}

uint64_t MainWindow::currentFolderParentId() {
    uint64_t parentId = 1;
    if (m_tree->get_selection()->get_selected()) {
        auto row = *(m_tree->get_selection()->get_selected());
        FolderModelColumns cols;
        parentId = row[cols.id];
    }
    return parentId;
}

void MainWindow::updateContents() {
    if (m_selection->get_selected()) {
        ContentsModelColumns cols;
        auto row = *(m_tree->get_selection()->get_selected());

        m_contents_store->clear();
        arc::Archive::instance().browse([&](sqlite3_uint64 id, const std::string& typ, const std::string& name, sqlite3_uint64 size, const std::string& hash) {

            auto irow = *m_contents_store->append();
            irow[cols.name] = name;
            irow[cols.typ] = Gdk::Pixbuf::create_from_resource(typ == "folder" ? "/icons/ca-folder-1.svg" : "/icons/ca-file.svg");
            irow[cols.id] = id;
            irow[cols.size] = (size == 0 ? Glib::ustring{} : Glib::ustring::compose("%1", size));
            irow[cols.hash] = hash;

        }, row[cols.id]);
    }
}

void MainWindow::updateMediaView() {

    if (!arc::Archive::instance().hasActiveArchive())
        return;

    m_media_store->clear();
    MediaListColumns cols;
    arc::Archive::instance().browseMedia([&](sqlite3_uint64 id, sqlite3_uint64 capacity, const std::string& name) {
        auto row = *m_media_store->append();
        if (id == arc::Archive::instance().settings->mediaId())
            row[cols.active_icon] = Gdk::Pixbuf::create_from_resource("/icons/ca-check.svg");
        row[cols.name] = name;
    });
}

