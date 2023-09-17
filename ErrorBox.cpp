//
// Created by developer on 9/17/23.
//

#include "ErrorBox.h"

ErrorBox::ErrorBox(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(win) {}

void ErrorBox::run(const cold_arc::Error& e) {
    auto r = runDialog<ErrorBox>("/main/err.glade", "err_dlg", e);
}
cold_arc::Result<> ErrorBox::construct(const Glib::RefPtr<Gtk::Builder>& builder, const cold_arc::Error& e) {
    auto buf = findObject<Gtk::TextBuffer>("textbuffer", builder);
    buf->set_text(cold_arc::explain_nested_error(e));

    auto css = Gtk::CssProvider::create();
    css->load_from_resource("/main/err.css");

    auto tv = findWidget<Gtk::TextView>("textview", builder);
    tv->get_style_context()->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    return {};
}
