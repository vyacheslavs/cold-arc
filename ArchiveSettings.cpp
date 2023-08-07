//
// Created by developer on 8/7/23.
//

#include "ArchiveSettings.h"
#include "Utils.h"
#include "Archive.h"

ArchiveSettings::ArchiveSettings(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    m_edit_arc_name = findWidget<Gtk::Entry>("edit_arc_name", builder);
    m_edit_arc_name->set_text(arc::Archive::instance().settings->name());
}

void ArchiveSettings::run() {

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/main/settings.glade");

    auto ret = findWidgetDerived<ArchiveSettings>("settings_win", builder);
    std::unique_ptr<ArchiveSettings> _auto(ret);

    if (static_cast<Gtk::Dialog*>(ret)->run() == Gtk::RESPONSE_OK) {
        arc::Archive::instance().settings->updateSettings(ret->getArchiveName());
    }
}

Glib::ustring ArchiveSettings::getArchiveName() const {
    return m_edit_arc_name->get_text();
}
