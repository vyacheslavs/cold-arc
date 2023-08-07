//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_ARCHIVESETTINGS_H
#define COLD_ARC_GTK_ARCHIVESETTINGS_H

#include <gtkmm-3.0/gtkmm.h>

class ArchiveSettings : public Gtk::Dialog {
    public:
        ArchiveSettings(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);
        static ArchiveSettings* create(const Glib::RefPtr<Gtk::Builder>& _builder);

};


#endif //COLD_ARC_GTK_ARCHIVESETTINGS_H
