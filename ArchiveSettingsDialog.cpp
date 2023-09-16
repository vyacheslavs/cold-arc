//
// Created by developer on 8/7/23.
//

#include "ArchiveSettingsDialog.h"
#include "Utils.h"
#include "Archive.h"

ArchiveSettingsDialog::ArchiveSettingsDialog(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    m_edit_arc_name = findWidget<Gtk::Entry>("edit_arc_name", builder);
    m_edit_arc_name->set_text(arc::Archive::instance().settings->name());
}

cold_arc::Result<> ArchiveSettingsDialog::run() {

    auto r = runDialog<ArchiveSettingsDialog>("/main/settings.glade", "settings_win");
    if (!r)
        return unexpected_nested(cold_arc::ErrorCode::ArchiveSettingsDialogError, r.error());

    if (r.value().rc == Gtk::RESPONSE_OK) {
        if (auto res = arc::Archive::instance().beginTransaction(); !res)
            return unexpected_nested(cold_arc::ErrorCode::ArchiveSettingsDialogError, res.error());

        if (auto res = arc::Archive::instance().settings->updateName(r.value().dialog->getArchiveName()); !res) {
            auto rb = arc::Archive::instance().rollbackTransaction();
            return unexpected_combined_error(cold_arc::ErrorCode::ArchiveSettingsDialogError, res.error(), rb.error());
        }
        if (auto res = arc::Archive::instance().commitTransaction(); !res)
            return unexpected_nested(cold_arc::ErrorCode::ArchiveSettingsDialogError, res.error());
    }
    return {};
}

Glib::ustring ArchiveSettingsDialog::getArchiveName() const {
    return m_edit_arc_name->get_text();
}
