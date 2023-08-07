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

void ArchiveSettingsDialog::run() {
    runDialog<ArchiveSettingsDialog>("/main/settings.glade", "settings_win", [](ArchiveSettingsDialog* dlg, int rc){
        if (rc == Gtk::RESPONSE_OK)
            arc::Archive::instance().settings->updateName(dlg->getArchiveName());
    });
}

Glib::ustring ArchiveSettingsDialog::getArchiveName() const {
    return m_edit_arc_name->get_text();
}
