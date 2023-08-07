//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_UTILS_H
#define COLD_ARC_GTK_UTILS_H

#include <gtkmm-3.0/gtkmm.h>

template <typename T>
T* findWidget(const Glib::ustring& name, const Glib::RefPtr<Gtk::Builder> &builder) {
    T* widget = nullptr;
    builder->get_widget<T>(name, widget);
    if (!widget)
        throw std::runtime_error(Glib::ustring::compose("failed to find widget %1", name));
    return widget;
}

template <typename T>
T* findWidgetDerived(const Glib::ustring& name, const Glib::RefPtr<Gtk::Builder> &builder) {
    T* widget = nullptr;
    builder->get_widget_derived<T>(name, widget);
    if (!widget)
        throw std::runtime_error(Glib::ustring::compose("failed to find widget %1", name));
    return widget;
}

bool endsWith(const Glib::ustring &filename, const Glib::ustring &postfix);
bool fileExists(const Glib::ustring& filename);

#endif //COLD_ARC_GTK_UTILS_H
