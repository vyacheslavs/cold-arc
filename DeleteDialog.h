//
// Created by developer on 8/17/23.
//

#ifndef COLD_ARC_GTK_DELETEDIALOG_H
#define COLD_ARC_GTK_DELETEDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "MediaView.h"

class DeleteDialog : public Gtk::Dialog {
    public:
        DeleteDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder, std::vector<uint64_t>&& _items, std::set<uint64_t>&& medias);

        static int run(std::vector<uint64_t>&& items, std::set<uint64_t>&& medias);

        std::vector<uint64_t> items;

    private:

        std::set<uint64_t> m_medias;
        MediaView* m_media_view;
};


#endif //COLD_ARC_GTK_DELETEDIALOG_H
