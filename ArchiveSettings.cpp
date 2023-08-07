//
// Created by developer on 8/7/23.
//

#include "ArchiveSettings.h"

ArchiveSettings::ArchiveSettings(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
}

ArchiveSettings* ArchiveSettings::create(const Glib::RefPtr<Gtk::Builder>& _builder) {
    ArchiveSettings* ret = nullptr;
    _builder->get_widget_derived("settings_win", ret);
    if (!ret)
        throw std::runtime_error("failed to initialize settings window");
    return ret;
}
