//
// Created by developer on 8/8/23.
//

#ifndef COLD_ARC_GTK_PROGRESSWINDOW_H
#define COLD_ARC_GTK_PROGRESSWINDOW_H

#include <gtkmm-3.0/gtkmm.h>
#include <thread>
#include <atomic>
#include <optional>
#include "ProgressInfo.h"
#include "Utils.h"

class ProgressWindow : public Gtk::Window {
    public:
        ProgressWindow(Gtk::Window::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

        bool isUploadInProgress() const;
        void startUploadProcess(const std::vector<Glib::RefPtr<Gio::File> >& files);

    private:
        Gtk::Button* m_hide_button;
        std::atomic_bool m_upload_in_progress {false};
        std::unique_ptr<std::thread> m_upload_thread;
        Glib::Dispatcher m_dispatcher;
        std::vector<Glib::RefPtr<Gio::File>> m_files;
        std::vector<UploadFileInfo> m_files_info;
        std::vector<std::string> m_failed_files;
        BodyGuard<ProgressInfo> m_progress_info;

        void on_thread_notification();
        void thread_main();
        std::string calculateSha256(const std::string& filename);
};


#endif //COLD_ARC_GTK_PROGRESSWINDOW_H
