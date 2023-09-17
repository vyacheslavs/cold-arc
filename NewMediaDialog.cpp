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
    m_rockridge = findWidget<Gtk::CheckButton>("rockridge_toggle", builder);
    m_joliet = findWidget<Gtk::CheckButton>("joliet_toggle", builder);

    m_edit_media_name->signal_changed().connect(sigc::mem_fun(this, &NewMediaDialog::onEditMediaNameChanged));
    m_edit_media_serial->signal_changed().connect(sigc::mem_fun(this, &NewMediaDialog::onEditMediaSerialChanged));
    m_edit_media_capacity->signal_changed().connect(sigc::mem_fun(this, &NewMediaDialog::onEditMediaCapacityChanged));

    m_edit_media_name->set_text("My CD/DVD");
    m_edit_media_serial->set_text(generateSerial());
    m_edit_media_capacity->set_value(750);

    checkSanity();
}

cold_arc::Result<> NewMediaDialog::run() {

    auto r = runDialog<NewMediaDialog>("/main/newmedia.glade", "settings_win");
    if (!r)
        return unexpected_nested(cold_arc::ErrorCode::NewMediaDialogError, r.error());
    if (r.value().rc == Gtk::RESPONSE_OK) {
        if (auto rb = arc::Archive::instance().beginTransaction(); !rb)
            return unexpected_nested(cold_arc::ErrorCode::NewMediaDialogError, rb.error());
        auto [name, serial, cap] = r.value().dialog->get();
        auto m = arc::Archive::instance().newMedia(name, serial, cap, r.value().dialog->rockridge(), r.value().dialog->joliet());
        if (!m) {
            auto rb = arc::Archive::instance().rollbackTransaction();
            return unexpected_combined_error(cold_arc::ErrorCode::NewMediaDialogError, m.error(), rb.error());
        }
        if (auto rb = arc::Archive::instance().commitTransaction(); !rb)
            return unexpected_nested(cold_arc::ErrorCode::NewMediaDialogError, rb.error());
    }
    return {};
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

std::tuple<Glib::ustring, Glib::ustring, uint64_t> NewMediaDialog::get() const {
    uint64_t cap = 1000'000ULL * m_edit_media_capacity->get_value_as_int();

    return {
            m_edit_media_name->get_text(),
            m_edit_media_serial->get_text(),
            cap
    };
}
bool NewMediaDialog::rockridge() const {
    return m_rockridge->get_active();
}
bool NewMediaDialog::joliet() const {
    return m_joliet->get_active();
}
