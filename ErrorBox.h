//
// Created by developer on 9/17/23.
//

#ifndef COLD_ARC_GTK_ERRORBOX_H
#define COLD_ARC_GTK_ERRORBOX_H

#include <gtkmm-3.0/gtkmm.h>
#include "Utils.h"

class ErrorBox : public Gtk::Dialog {
    public:
        ErrorBox(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

        static void run(const cold_arc::Error& e);
        [[nodiscard]] cold_arc::Result<> construct(const Glib::RefPtr<Gtk::Builder>& builder, const cold_arc::Error& e);
};


#endif //COLD_ARC_GTK_ERRORBOX_H
