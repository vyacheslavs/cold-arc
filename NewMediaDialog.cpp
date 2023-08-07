//
// Created by developer on 8/7/23.
//

#include <iostream>
#include "NewMediaDialog.h"
#include "Utils.h"
#include "Archive.h"

NewMediaDialog::NewMediaDialog(Gtk::Dialog::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Dialog(win) {
    m_edit_media_name = findWidget<Gtk::Entry>("edit_media_name", builder);
    m_edit_media_serial = findWidget<Gtk::Entry>("edit_media_serial", builder);
    m_edit_media_capacity = findWidget<Gtk::SpinButton>("edit_media_capacity", builder);
    m_edit_media_name->signal_changed().connect(sigc::mem_fun(this, &NewMediaDialog::onEditMediaNameChanged));
    m_edit_media_serial->signal_changed().connect(sigc::mem_fun(this, &NewMediaDialog::onEditMediaSerialChanged));
    m_edit_media_capacity->signal_changed().connect(sigc::mem_fun(this, &NewMediaDialog::onEditMediaCapacityChanged));

    m_edit_media_name->set_text("My CD/DVD");
    m_edit_media_serial->set_text(generateSerial());
    m_edit_media_capacity->set_value(750);

    set_response_sensitive(Gtk::RESPONSE_OK, false);
}

void NewMediaDialog::run() {
    runDialog<NewMediaDialog>("/main/newmedia.glade", "settings_win", [](NewMediaDialog* dlg, int rc){
        if (rc == Gtk::RESPONSE_OK) {
            auto [name, serial, cap] = dlg->get();
            arc::Archive::instance().newMedia(name, serial, cap);
        }
    });
}

void NewMediaDialog::onEditMediaNameChanged() {
    checkSanity();
}

void NewMediaDialog::onEditMediaSerialChanged() {
    checkSanity();
}

void NewMediaDialog::onEditMediaCapacityChanged() {
    checkSanity();
}

void NewMediaDialog::checkSanity() {

    if (!m_edit_media_capacity->get_text().empty()) {
        const auto val = std::stoll(m_edit_media_capacity->get_text());
        m_edit_media_capacity->set_value(val);
    }

    set_response_sensitive(Gtk::RESPONSE_OK,
                           !m_edit_media_name->get_text().empty() &&
                           !m_edit_media_serial->get_text().empty() &&
                           m_edit_media_capacity->get_value() > 0
    );
}

std::tuple<Glib::ustring, Glib::ustring, int> NewMediaDialog::get() const {
    return {
            m_edit_media_name->get_text(),
            m_edit_media_serial->get_text(),
            m_edit_media_capacity->get_value_as_int()
    };
}
