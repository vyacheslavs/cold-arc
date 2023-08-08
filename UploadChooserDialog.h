//
// Created by developer on 8/8/23.
//

#ifndef COLD_ARC_GTK_UPLOADCHOOSERDIALOG_H
#define COLD_ARC_GTK_UPLOADCHOOSERDIALOG_H

#include <gtkmm-3.0/gtkmm.h>

class UploadChooserDialog : public Gtk::Dialog {
    public:

    UploadChooserDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);
    static void run();

    std::vector< Glib::RefPtr<Gio::File> > getFiles() const;

    private:

        Gtk::FileChooserWidget* m_chooser_widget {nullptr};
};


#endif //COLD_ARC_GTK_UPLOADCHOOSERDIALOG_H
