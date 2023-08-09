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

        void startUploadProcess(const std::vector<Glib::RefPtr<Gio::File> >& files);

        void tryShow();
        void doShow();

    private:
        Gtk::Button* m_hide_button;
        std::unique_ptr<std::thread> m_upload_thread;
        Glib::Dispatcher m_dispatcher;
        std::vector<Glib::RefPtr<Gio::File>> m_files;
        std::vector<UploadFileInfo> m_files_info;
        std::vector<std::string> m_failed_files;
        BodyGuard<ProgressInfo> m_progress_info;
        Gtk::ProgressBar* m_current_action;
        Gtk::ProgressBar* m_total_progress;
        Gtk::Label* m_current_path;
        bool m_hidden{false};

        void onHideButtonClicked();
        void onThreadNotification();
        bool onCloseButtonClicked(GdkEventAny*);
        void thread_main();
        static std::string calculateSha256(const std::string& filename, uint64_t size, const std::function<void(uint64_t)>& callback);
};


#endif //COLD_ARC_GTK_PROGRESSWINDOW_H
