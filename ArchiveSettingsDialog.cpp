//
// Created by developer on 8/7/23.
//

#include "ArchiveSettingsDialog.h"
#include "Utils.h"
#include "Archive.h"

ArchiveSettingsDialog::ArchiveSettingsDialog(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    m_edit_arc_name = findWidget<Gtk::Entry>("edit_arc_name", builder);
    m_edit_arc_name->set_text(arc::Archive::instance().settings->name());
    m_paranoic = findWidget<Gtk::ToggleButton>("paranoic", builder);
    m_paranoic->set_active(arc::Archive::instance().settings->is_paranoic());
}

cold_arc::Result<> ArchiveSettingsDialog::run() {

    auto r = runDialog<ArchiveSettingsDialog>("/main/settings.glade", "settings_win");
    if (!r)
        return unexpected_nested(cold_arc::ErrorCode::ArchiveSettingsDialogError, r.error());

    if (r.value().rc == Gtk::RESPONSE_OK) {

        if (arc::Archive::instance().settings->is_paranoic()) {
            std::stringstream ss;
            ss << "Save point before settings change";
            if (auto res = arc::Archive::instance().commit(ss.str()); !res)
                return unexpected_nested(cold_arc::ErrorCode::ArchiveSettingsDialogError, res.error());
        }

        if (auto res = arc::Archive::instance().beginTransaction(); !res)
            return unexpected_nested(cold_arc::ErrorCode::ArchiveSettingsDialogError, res.error());

        if (auto res = arc::Archive::instance().settings->update(r.value().dialog->getArchiveName(), r.value().dialog->m_paranoic->get_active()); !res) {
            auto rb = arc::Archive::instance().rollbackTransaction();
            return unexpected_combined_error(cold_arc::ErrorCode::ArchiveSettingsDialogError, res.error(), rb);
        }
        if (auto res = arc::Archive::instance().commitTransaction(); !res)
            return unexpected_nested(cold_arc::ErrorCode::ArchiveSettingsDialogError, res.error());
    }
    return {};
}

Glib::ustring ArchiveSettingsDialog::getArchiveName() const {
    return m_edit_arc_name->get_text();
}
