//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_SIGNALS_H
#define COLD_ARC_GTK_SIGNALS_H

#include <sigc++/sigc++.h>
#include <gtkmm-3.0/gtkmm.h>

class Signals {
    public:
        static Signals& instance();

        sigc::signal<void> update_main_window;
        sigc::signal<void> update_tree;
        sigc::signal<void> update_media_view;
        sigc::signal<void(const Glib::ustring& folder_name, uint64_t id, uint64_t parentId)> new_folder;
        sigc::signal<void(const std::vector<Glib::RefPtr<Gio::File> >&)> upload_files;
};


#endif //COLD_ARC_GTK_SIGNALS_H
