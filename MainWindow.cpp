//
// Created by developer on 8/5/23.
//

#include "MainWindow.h"
#include "Archive.h"
#include "ArchiveSettingsDialog.h"
#include "Utils.h"
#include "Signals.h"
#include "NewFolderDialog.h"
#include "FolderModelColumns.h"
#include "UploadChooserDialog.h"
#include "UploadDialog.h"
#include "ContentsModelColumns.h"
#include "DeleteDialog.h"
#include "MediaView.h"

MainWindow::MainWindow(Gtk::Window::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Window(win),
                                                                                                      m_builder(
                                                                                                          builder) {

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
    m_media_view = findWidgetDerived<MediaView>("mediaview", m_builder);
    m_sep1 = findWidget<Gtk::SeparatorToolItem>("sep1", m_builder);
    m_sep2 = findWidget<Gtk::SeparatorToolItem>("sep2", m_builder);
    m_delete_button = findWidget<Gtk::ToolButton>("delete_button", builder);
    m_contents_selection = findObject<Gtk::TreeSelection>("contents_selection", builder);

    applyFontAwesome(m_open_archive_button->get_label_widget());
    applyFontAwesome(m_new_archive_button->get_label_widget());
    applyFontAwesome(m_add_new_media_button->get_label_widget());
    applyFontAwesome(m_archive_settings_button->get_label_widget());
    applyFontAwesome(m_create_folder->get_label_widget());
    applyFontAwesome(m_upload_button->get_label_widget());
    applyFontAwesome(m_delete_button->get_label_widget());

    m_new_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewArchiveButtonClicked));
    m_open_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onOpenArchiveButtonClicked));
    m_archive_settings_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onArchiveSettings));
    m_add_new_media_button->signal_clicked().connect(sigc::mem_fun(m_media_view, &MediaView::onNewMediaButtonClicked));
    m_create_folder->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onCreateFolderClicked));
    m_upload_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onUploadButtonClicked));
    m_selection->signal_changed().connect(sigc::mem_fun(this, &MainWindow::updateContents));
    m_tree->signal_row_collapsed().connect(sigc::mem_fun(this, &MainWindow::onTreeViewRowCollapsed));
    m_tree->signal_row_expanded().connect(sigc::mem_fun(this, &MainWindow::onTreeViewRowExpanded));
    m_delete_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onDeleteButtonClicked));
    m_contents_selection->signal_changed().connect(sigc::mem_fun(this, &MainWindow::updateContentsSelection));
    m_media_view->mediaToggled.connect(sigc::mem_fun(this, &MainWindow::updateTree));

    Signals::instance().update_main_window.connect(sigc::mem_fun(this, &MainWindow::updateUI));
    Signals::instance().new_folder.connect(sigc::mem_fun(this, &MainWindow::allocateTreeNodeUsingParentId));
    Signals::instance().update_tree.connect(sigc::mem_fun(this, &MainWindow::updateTree));
    Signals::instance().update_media_view.connect(sigc::mem_fun(m_media_view, &MediaView::update));

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
        m_contents_view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);
    }

    updateUI();
    updateTree();
    m_media_view->update();
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

        if (auto res = arc::Archive::instance().newArchive(newArcDlg.get_filename()); !res) {
            bool is_critical = true;
            if (cold_arc::get_nested<0>(res.error()).code == cold_arc::ErrorCode::NewArchiveExtractError &&
                cold_arc::get_nested<1>(res.error()).code == cold_arc::ErrorCode::ExtractTargetCreateError)
                is_critical = false;

            reportError(res.error(), is_critical);
        }
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
        if (auto res = arc::Archive::instance().openArchive(openArcDlg.get_filename()); !res) {
            reportError(res.error());
        }
    }
}

void MainWindow::updateUI() {

    auto theme = Gtk::IconTheme::get_default();
    theme->add_resource_path("/icons/app");

    m_sep1->set_visible(arc::Archive::instance().hasActiveArchive());
    m_sep2->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_upload_button->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_create_folder->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_archive_settings_button->set_visible(arc::Archive::instance().hasActiveArchive());
    m_add_new_media_button->set_visible(arc::Archive::instance().hasActiveArchive());
    m_delete_button->set_visible(arc::Archive::instance().hasActiveArchive());
    m_delete_button->set_sensitive(false);

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
    if (auto res = ArchiveSettingsDialog::run(); !res)
        reportError(res.error());
}

void MainWindow::onCreateFolderClicked() {
    if (auto res = NewFolderDialog::run(currentFolderParentId()); !res)
        reportError(res.error());
}

void MainWindow::updateTree() {

    if (!arc::Archive::instance().hasActiveArchive())
        return;

    m_tree_store->clear();
    m_contents_store->clear();
    m_tree_fast_access.clear();
    m_colapse_expand_records_cached = std::move(m_colapse_expand_records);

    if (auto res = arc::Archive::instance().walkTree(
        [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk,
            sqlite3_uint64 dt, sqlite3_uint64 parent_id) {
            allocateTreeNodeUsingParentId(name, id, parent_id);
            return cold_arc::Error{};
        }, 0, m_media_view->collectExclusions()); !res) {
        reportError(res.error());
    }

    for (const auto& exp : m_colapse_expand_records_cached) {
        if (exp.second) {
            auto it = m_tree_fast_access.find(exp.first);
            if (it != m_tree_fast_access.end()) {
                m_tree->expand_to_path(m_tree_store->get_path(it->second));
                m_colapse_expand_records[exp.first] = true;
            }
        }
    }
}

void MainWindow::allocateTreeNodeUsingParentId(const Glib::ustring& name, uint64_t id, uint64_t parent_id) {
    if (parent_id == 0) {
        auto node = m_tree_store->append();
        allocateTreeNode(node, id, name);

    } else {
        auto t_it = m_tree_fast_access.find(parent_id);
        if (t_it != m_tree_fast_access.end()) {
            auto node = m_tree_store->append(t_it->second->children());
            allocateTreeNode(node, id, name);
        }
    }
}

void MainWindow::onUploadButtonClicked() {
    auto files = UploadChooserDialog::run();
    if (!files) {
        reportError(files.error());
        return;
    }
    if (!files.value().empty()) {
        auto res = UploadDialog::run(currentFolderParentId(), std::move(files.value()));
        if (!res) {
            reportError(res.error());
            return;
        }
        if (res.value()) {
            updateTree();
            m_media_view->update();
        }
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
        auto res = arc::Archive::instance().browse([&](sqlite3_uint64 id, const std::string& typ, const std::string& name, sqlite3_uint64 size, const std::string& hash, const std::string& lnk) -> cold_arc::Error {
            auto irow = *m_contents_store->append();
            irow[cols.name] = name;
            irow[cols.typ] = Gdk::Pixbuf::create_from_resource(typ == "folder" ? "/icons/ca-folder-1.svg" : "/icons/ca-file.svg");
            irow[cols.id] = id;
            irow[cols.size] = (size == 0 ? Glib::ustring{} : Glib::ustring::compose("%1", size));
            irow[cols.hash] = hash;
            return cold_arc::Error{};
        }, row[cols.id], m_media_view->collectExclusions());

        if (!res)
            reportError(res.error());
    }
}

void MainWindow::onTreeViewRowCollapsed(const Gtk::TreeIter& iter, const Gtk::TreePath& path) {
    FolderModelColumns cols;
    if (iter != m_tree_store->children().end()) {
        const auto& row = (*iter);
        m_colapse_expand_records[row[cols.id]] = false;
    }
}

void MainWindow::onTreeViewRowExpanded(const Gtk::TreeIter& iter, const Gtk::TreePath& path) {
    FolderModelColumns cols;
    if (iter != m_tree_store->children().end()) {
        const auto& row = (*iter);
        m_colapse_expand_records[row[cols.id]] = true;
    }
}

void MainWindow::onDeleteButtonClicked() {
    std::set<uint64_t> medias;
    std::vector<uint64_t> items;

    m_contents_selection->selected_foreach_iter([&](const auto& it) {
        const auto& row = *it;
        ContentsModelColumns cols;

        uint64_t arc_id = row[cols.id];
        items.push_back(arc_id);

        if (auto res = arc::Archive::instance().getMediaForArcId(arc_id, [&](sqlite3_uint64 media_id) {
            medias.insert(media_id);
            return cold_arc::Error{};
        }); !res) {
            reportError(res.error());
            return;
        }
    });
    if (auto res = DeleteDialog::run(std::move(items), std::move(medias)); !res)
        reportError(res.error());

}

void MainWindow::updateContentsSelection() {
    m_delete_button->set_sensitive(m_contents_view->get_selection()->count_selected_rows() > 0);
}
