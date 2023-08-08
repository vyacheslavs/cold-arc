//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_SIGNALS_H
#define COLD_ARC_GTK_SIGNALS_H

#include <sigc++/sigc++.h>
#include <glibmm/ustring.h>

class Signals {
    public:
        static Signals& instance();

        sigc::signal<void> update_main_window;
        sigc::signal<void> update_tree;
        sigc::signal<void(const Glib::ustring& folder_name, uint64_t id, uint64_t parentId)> new_folder;
};


#endif //COLD_ARC_GTK_SIGNALS_H
