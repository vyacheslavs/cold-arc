//
// Created by developer on 8/5/23.
//

#ifndef COLD_ARC_GTK_MAINWINDOW_H
#define COLD_ARC_GTK_MAINWINDOW_H

#include <gtkmm-3.0/gtkmm.h>

class MainWindow : public Gtk::Window {
public:
    MainWindow(Gtk::Window::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

private:

    const Glib::RefPtr<Gtk::Builder>& m_builder;
};


#endif //COLD_ARC_GTK_MAINWINDOW_H
