//
// Created by developer on 8/8/23.
//

#ifndef COLD_ARC_GTK_UPLOADCHOOSERDIALOG_H
#define COLD_ARC_GTK_UPLOADCHOOSERDIALOG_H

#include <gtkmm-3.0/gtkmm.h>

#include "UploadFilesCollection.h"
#include "Utils.h"

class UploadChooserDialog : public Gtk::Dialog {
    public:

    UploadChooserDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);
    [[nodiscard]] static cold_arc::Result<UploadFilesCollection> run();

    UploadFilesCollection getFiles() const;

    private:

        Gtk::FileChooserWidget* m_chooser_widget {nullptr};
};


#endif //COLD_ARC_GTK_UPLOADCHOOSERDIALOG_H
