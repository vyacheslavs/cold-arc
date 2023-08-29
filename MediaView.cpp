//
// Created by developer on 8/17/23.
//

#include "MediaView.h"
#include "Utils.h"
#include "MediaListColumns.h"
#include "Archive.h"
#include "NewMediaDialog.h"
#include "ExportDialog.h"
#include "Signals.h"

#define LIBISOFS_WITHOUT_LIBBURN yes
#include "libisofs.h"

MediaView::MediaView(Gtk::TreeView::BaseObjectType* bot, const Glib::RefPtr<Gtk::Builder>& builder, bool toolbar) {
    m_media_view = findWidget<Gtk::TreeView>("mediaview", builder);
    m_media_store = findObject<Gtk::ListStore>("liststore2", builder);

    MediaListColumns cols;
    m_media_view->append_column("", cols.active_icon);
    auto comboRenderer = Gtk::manage(new Gtk::CellRendererToggle);
    Gtk::TreeView::Column* pColumn = Gtk::manage(new Gtk::TreeView::Column(""));
    pColumn->pack_start(*comboRenderer, false);
    m_media_view->append_column(*pColumn);
    pColumn->add_attribute(comboRenderer->property_active(), cols.checkbox);
    comboRenderer->set_activatable(true);
    comboRenderer->signal_toggled().connect(sigc::mem_fun(this, &MediaView::onMediaToggle));

    m_media_view->append_column("Name", cols.name);
    m_media_view->append_column("Serial", cols.serial);

    auto progressRenderer = Gtk::manage(new Gtk::CellRendererProgress);
    Gtk::TreeView::Column* progressCol = Gtk::manage(new Gtk::TreeView::Column("Space"));
    progressCol->pack_start(*progressRenderer, false);
    m_media_view->append_column(*progressCol);
    progressCol->add_attribute(progressRenderer->property_value(), cols.percentage);
    progressCol->add_attribute(progressRenderer->property_text(), cols.percentage_text);

    if (!toolbar)
        return;

    m_media_toolbar = findWidget<Gtk::Toolbar>("media_toolbar", builder);
    m_media_view_selection = findObject<Gtk::TreeSelection>("media_view_selection", builder);
    m_media_view_select_button = findWidget<Gtk::ToolButton>("select_button", builder);
    m_media_view_remove_button = findWidget<Gtk::ToolButton>("remove_button", builder);
    m_media_new_button = findWidget<Gtk::ToolButton>("new_media_button", builder);
    m_media_export_button = findWidget<Gtk::ToolButton>("export_button", builder);

    applyFontAwesome(m_media_view_select_button->get_label_widget(), false);
    applyFontAwesome(m_media_view_remove_button->get_label_widget(), false);
    applyFontAwesome(m_media_new_button->get_label_widget(), false);
    applyFontAwesome(m_media_export_button->get_label_widget(), false);

    m_media_view_select_button->set_sensitive(false);
    m_media_view_remove_button->set_sensitive(false);
    m_media_export_button->set_sensitive(false);

    m_media_toolbar->set_visible(false);
    m_media_view_selection->signal_changed().connect(sigc::mem_fun(this, &MediaView::onMediaViewSelectionChanged));
    m_media_view_select_button->signal_clicked().connect(sigc::mem_fun(this, &MediaView::onMediaViewSelectButton));
    m_media_view_remove_button->signal_clicked().connect(sigc::mem_fun(this, &MediaView::onMediaViewRemoveButtonClicked));
    m_media_new_button->signal_clicked().connect(sigc::mem_fun(this, &MediaView::onNewMediaButtonClicked));
    m_media_export_button->signal_clicked().connect(sigc::mem_fun(this, &MediaView::onMediaViewExportButton));
}

void MediaView::onMediaToggle(const Glib::ustring& path) {
    MediaListColumns cols;
    auto it = m_media_store->get_iter(path);
    if (it != m_media_store->children().end()) {
        const auto& row = *it;
        row[cols.checkbox] = !row[cols.checkbox];
        mediaToggled.emit();
    }
}

void MediaView::update() {
    try {
        if (!arc::Archive::instance().hasActiveArchive())
            return;

        m_media_toolbar->set_visible(arc::Archive::instance().hasCurrentMedia());

        m_media_store->clear();
        MediaListColumns cols;
        arc::Archive::instance().browseMedia(sigc::mem_fun(this, &MediaView::addMedia));
    } catch (const sqlite::sqlite_exception& e) {
        sqliteError(e);
    }
}

std::string MediaView::collectExclusions() const {
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

void MediaView::onMediaViewSelectionChanged() {
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
    m_media_export_button->set_sensitive(outcome);
}

void MediaView::onMediaViewSelectButton() {
    auto sel = m_media_view_selection->get_selected();
    if (!sel)
        return;

    MediaListColumns cols;
    auto new_id = (*sel)[cols.id];
    auto p = arc::Archive::instance().savePoint();
    try {
        arc::Archive::instance().settings->switchMedia(new_id);
    } catch (const sqlite::sqlite_exception& e) {
        p.rollback();
        sqliteError(e);
    }
}

void MediaView::onMediaViewRemoveButtonClicked() {
    auto sel = m_media_view_selection->get_selected();
    if (!sel)
        return;

    MediaListColumns cols;
    auto media = arc::Archive::instance().settings->media()->getMedia((*sel)[cols.id]);
    Gtk::MessageDialog dlg("Are you sure you want to delete this media?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
    dlg.set_secondary_text(Glib::ustring::compose("All files and folders from this media (%1) will be deleted from the archive!", media->name()));

    if (dlg.run() == Gtk::RESPONSE_NO)
        return;

    auto p = arc::Archive::instance().savePoint();
    try {
        media->remove();
    } catch (const sqlite::sqlite_exception& e) {
        p.rollback();
        sqliteError(e);
    }
}

void MediaView::onNewMediaButtonClicked() {
    NewMediaDialog::run();
}
void MediaView::addMedia(uint64_t id, uint64_t capacity, uint64_t occupied, const std::string& name, const std::string& serial) {
    MediaListColumns cols;

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
}
std::vector<uint64_t> MediaView::collectCheckedIds() const {
    std::vector<uint64_t> ids;

    MediaListColumns cols;
    for (const auto& m: m_media_store->children()) {
        if (m[cols.checkbox]) {
            ids.push_back(m[cols.id]);
        }
    }
    return ids;
}
void MediaView::onMediaViewExportButton() {
    auto sel = m_media_view_selection->get_selected();
    if (!sel)
        return;

    MediaListColumns cols;
    uint64_t media_id = (*sel)[cols.id];

    Gtk::FileChooserDialog newISO("Please choose the name of ISO file", Gtk::FILE_CHOOSER_ACTION_SAVE);

    newISO.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    newISO.add_button("Save", Gtk::RESPONSE_OK);

    auto isoFilter = Gtk::FileFilter::create();
    isoFilter->set_name("iso files");
    isoFilter->add_pattern("*.iso");
    newISO.add_filter(isoFilter);

    if (newISO.run() == Gtk::RESPONSE_OK) {
        newISO.hide();
        ExportDialog::run(media_id, newISO.get_filename());
    }
}

