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

    auto applyFontAwesome = [&](auto widget, bool resize = true) {
        auto desc = widget->get_pango_context()->get_font_description();
        desc.set_family("Font Awesome 6 Free");
        if (resize)
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
    m_sep1 = findWidget<Gtk::SeparatorToolItem>("sep1", m_builder);
    m_sep2 = findWidget<Gtk::SeparatorToolItem>("sep2", m_builder);
    m_media_toolbar = findWidget<Gtk::Toolbar>("media_toolbar", m_builder);
    m_media_new_button = findWidget<Gtk::ToolButton>("new_media_button", m_builder);
    m_media_view_selection = findObject<Gtk::TreeSelection>("media_view_selection", m_builder);
    m_media_view_select_button = findWidget<Gtk::ToolButton>("select_button", m_builder);
    m_media_view_remove_button = findWidget<Gtk::ToolButton>("remove_button", m_builder);

    applyFontAwesome(m_open_archive_button->get_label_widget());
    applyFontAwesome(m_new_archive_button->get_label_widget());
    applyFontAwesome(m_add_new_media_button->get_label_widget());
    applyFontAwesome(m_archive_settings_button->get_label_widget());
    applyFontAwesome(m_create_folder->get_label_widget());
    applyFontAwesome(m_upload_button->get_label_widget());
    applyFontAwesome(m_media_new_button->get_label_widget(), false);
    applyFontAwesome(m_media_view_select_button->get_label_widget(), false);
    applyFontAwesome(m_media_view_remove_button->get_label_widget(), false);

    m_new_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewArchiveButtonClicked));
    m_open_archive_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onOpenArchiveButtonClicked));
    m_archive_settings_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onArchiveSettings));
    m_add_new_media_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewMediaButtonClicked));
    m_create_folder->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onCreateFolderClicked));
    m_upload_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onUploadButtonClicked));
    m_selection->signal_changed().connect(sigc::mem_fun(this, &MainWindow::updateContents));
    m_media_new_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onNewMediaButtonClicked));
    m_media_view_selection->signal_changed().connect(sigc::mem_fun(this, &MainWindow::onMediaViewSelectionChanged));
    m_media_view_select_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onMediaViewSelectButton));
    m_media_view_remove_button->signal_clicked().connect(sigc::mem_fun(this, &MainWindow::onMediaViewRemoveButtonClicked));
    m_tree->signal_row_collapsed().connect(sigc::mem_fun(this, &MainWindow::onTreeViewRowCollapsed));
    m_tree->signal_row_expanded().connect(sigc::mem_fun(this, &MainWindow::onTreeViewRowExpanded));

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
        auto comboRenderer = Gtk::manage(new Gtk::CellRendererToggle);
        Gtk::TreeView::Column* pColumn = Gtk::manage(new Gtk::TreeView::Column(""));
        pColumn->pack_start(*comboRenderer, false);
        m_media_view->append_column(*pColumn);
        pColumn->add_attribute(comboRenderer->property_active(), cols.checkbox);
        comboRenderer->set_activatable(true);
        comboRenderer->signal_toggled().connect(sigc::mem_fun(this, &MainWindow::onMediaToggle));

        m_media_view->append_column("Name", cols.name);
        m_media_view->append_column("Serial", cols.serial);

        auto progressRenderer = Gtk::manage(new Gtk::CellRendererProgress);
        Gtk::TreeView::Column* progressCol = Gtk::manage(new Gtk::TreeView::Column("Space"));
        progressCol->pack_start(*progressRenderer, false);
        m_media_view->append_column(*progressCol);
        progressCol->add_attribute(progressRenderer->property_value(), cols.percentage);
        progressCol->add_attribute(progressRenderer->property_text(), cols.percentage_text);
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

    m_sep1->set_visible(arc::Archive::instance().hasActiveArchive());
    m_sep2->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_upload_button->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_create_folder->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_media_toolbar->set_visible(arc::Archive::instance().hasCurrentMedia());
    m_archive_settings_button->set_visible(arc::Archive::instance().hasActiveArchive());
    m_add_new_media_button->set_visible(arc::Archive::instance().hasActiveArchive());

    onMediaViewSelectionChanged();

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
    m_tree_fast_access.clear();
    m_colapse_expand_records_cached = std::move(m_colapse_expand_records);

    arc::Archive::instance().walkTree(
        [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk,
            sqlite3_uint64 dt, sqlite3_uint64 parent_id) {
            allocateTreeNodeUsingParentId(name, id, parent_id);
        }, 0, collectExclusions());
}

std::string MainWindow::collectExclusions() {
    std::string exclustions;
    bool at_least_one_exclusion = false;

    MediaListColumns cols;
    for (const auto& m: m_media_store->children()) {
        if (m[cols.checkbox]) {
            if (!exclustions.empty()) exclustions.append(",");
            exclustions.append(std::to_string(m[cols.id]));
        } else
            at_least_one_exclusion = true;
    }
    if (!at_least_one_exclusion)
        exclustions.clear();
    return exclustions;
}

void MainWindow::allocateTreeNodeUsingParentId(const Glib::ustring& name, uint64_t id, uint64_t parent_id) {
    if (parent_id == 0) {
        allocateTreeNode(m_tree_store->append(), id, name);
    } else {
        auto t_it = m_tree_fast_access.find(parent_id);
        if (t_it != m_tree_fast_access.end()) {
            allocateTreeNode(m_tree_store->append(t_it->second->children()), id, name);

            auto cached = m_colapse_expand_records_cached.find(id);
            if (cached != m_colapse_expand_records_cached.end() && cached->second) {
                m_tree->expand_row(m_tree_store->get_path(t_it->second), true);
                m_colapse_expand_records[id] = true;
            }
        }
    }
}

void MainWindow::onUploadButtonClicked() {
    auto files = UploadChooserDialog::run();
    if (!files.empty() && UploadDialog::run(currentFolderParentId(), std::move(files))) {
        updateTree();
        updateMediaView();
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

        }, row[cols.id], collectExclusions());
    }
}

void MainWindow::updateMediaView() {

    if (!arc::Archive::instance().hasActiveArchive())
        return;

    m_media_store->clear();
    MediaListColumns cols;
    arc::Archive::instance().browseMedia([&](sqlite3_uint64 id, sqlite3_uint64 capacity, sqlite3_uint64 occupied, const std::string& name, const std::string& serial) {
        auto row = *m_media_store->append();
        if (id == arc::Archive::instance().settings->media()->id())
            row[cols.active_icon] = Gdk::Pixbuf::create_from_resource("/icons/ca-check.svg");
        row[cols.name] = name;
        row[cols.checkbox] = 1;
        row[cols.serial] = serial;
        row[cols.id] = id;
        row[cols.percentage] = static_cast<int>((100 * occupied) / capacity);
        std::ostringstream ss;
        ss << HumanReadable{occupied} << "/" << HumanReadable{capacity};
        row[cols.percentage_text] = ss.str();
    });
}

void MainWindow::onMediaToggle(const Glib::ustring& path) {
    MediaListColumns cols;
    auto it = m_media_store->get_iter(path);
    if (it != m_media_store->children().end()) {
        const auto& row = *it;
        row[cols.checkbox] = !row[cols.checkbox];
        updateTree();
    }
}

void MainWindow::onMediaViewSelectionChanged() {
    auto sel = m_media_view_selection->get_selected();
    auto outcome = false;
    if (!sel) {
        outcome = false;
    } else {
        const auto& row = *sel;
        MediaListColumns cols;
        outcome = row[cols.id] != arc::Archive::instance().settings->media()->id();
    }
    m_media_view_select_button->set_sensitive(outcome);
    m_media_view_remove_button->set_sensitive(outcome);
}

void MainWindow::onMediaViewSelectButton() {
    auto sel = m_media_view_selection->get_selected();
    if (!sel)
        return;

    MediaListColumns cols;
    auto new_id = (*sel)[cols.id];
    arc::Archive::instance().settings->switchMedia(new_id);
}

void MainWindow::onMediaViewRemoveButtonClicked() {
    auto sel = m_media_view_selection->get_selected();
    if (!sel)
        return;

    MediaListColumns cols;
    auto media = arc::Archive::instance().settings->media()->getMedia((*sel)[cols.id]);
    Gtk::MessageDialog dlg("Are you sure you want to delete this media?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
    dlg.set_secondary_text(Glib::ustring::compose("All files and folders from this media (%1) will be deleted from the archive!", media->name()));

    if (dlg.run() == Gtk::RESPONSE_NO)
        return;

    media->remove();
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

