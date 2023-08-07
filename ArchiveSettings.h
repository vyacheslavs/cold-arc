//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_ARCHIVESETTINGS_H
#define COLD_ARC_GTK_ARCHIVESETTINGS_H

#include <gtkmm-3.0/gtkmm.h>

class ArchiveSettings : public Gtk::Dialog {
    public:
        ArchiveSettings(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);
        static void run();

        Glib::ustring getArchiveName() const;
    private:

        Gtk::Entry* m_edit_arc_name;
};


#endif //COLD_ARC_GTK_ARCHIVESETTINGS_H
