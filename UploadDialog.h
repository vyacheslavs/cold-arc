//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADDIALOG_H
#define COLD_ARC_GTK_UPLOADDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "UploadFilesCollection.h"

class UploadDialog : public Gtk::Dialog {
public:
    UploadDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder,
                 UploadFilesCollection&& files);

    static void run(UploadFilesCollection&& files);

private:

    UploadFilesCollection m_files;
};


#endif //COLD_ARC_GTK_UPLOADDIALOG_H
