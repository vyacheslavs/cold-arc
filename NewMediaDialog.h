//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_NEWMEDIADIALOG_H
#define COLD_ARC_GTK_NEWMEDIADIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "Utils.h"

class NewMediaDialog : public Gtk::Dialog {
public:
    NewMediaDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);
    [[nodiscard]] static cold_arc::Result<> run();

    std::tuple<Glib::ustring, Glib::ustring, uint64_t> get() const;

    bool rockridge() const;
    bool joliet() const;

private:

    void onEditMediaNameChanged();
    void onEditMediaSerialChanged();
    void onEditMediaCapacityChanged();

    Gtk::Entry* m_edit_media_name;
    Gtk::Entry* m_edit_media_serial;
    Gtk::SpinButton* m_edit_media_capacity;
    Gtk::CheckButton* m_rockridge;
    Gtk::CheckButton* m_joliet;

    void checkSanity();

};


#endif //COLD_ARC_GTK_NEWMEDIADIALOG_H
