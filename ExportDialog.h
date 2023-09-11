//
// Created by developer on 9/7/23.
//

#ifndef COLD_ARC_GTK_EXPORTDIALOG_H
#define COLD_ARC_GTK_EXPORTDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "ISOBuilder.h"
#include "Dispatcher.h"
#include "Exceptions.h"
#include "Utils.h"
#include <thread>

enum class progress_type {
        unknown,
        overall,
        last_file,
};

struct ExportProgressInfo {
    bool running {true};
    std::shared_ptr<ExceptionCargoBase> e;

    progress_type prog_type {progress_type::unknown};
    int progress {0}; // overall progress
    std::string file_name{0};

    explicit ExportProgressInfo(bool is_running) : running(is_running) {}
    explicit ExportProgressInfo(std::string fn, int prog) : running(true), prog_type(progress_type::last_file), progress(prog), file_name(std::move(fn)) {}
    ExportProgressInfo(int prog, std::string fn) : running(true), prog_type(progress_type::overall), progress(prog), file_name(std::move(fn)) {}

    template <typename E>
    explicit ExportProgressInfo(const E& _e) : running(false), e(std::make_shared<ExceptionCargo<E>>(_e)) {}
};

using ExportProgressInfoQueue = std::list<ExportProgressInfo>;
using ExportProgressInfoQueueSafe = BodyGuard<ExportProgressInfoQueue>;

class ExportDialog : public Gtk::Dialog {
    public:
        ExportDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder, uint64_t media_id, std::string  iso_file);

        static void run(uint64_t media_id, const std::string& iso_file);

    private:

        template<typename E>
        void notifyThreadStopped(E&& e) {
            {
                auto cc = m_queue.access();
                cc->emplace_back(std::forward<E>(e));
            }
            m_dispatcher.emit(DispatcherEmitPolicy::Force);
        }

        void notifyProgress(int progress, std::string file);
        void notifyLastFileProgress(int progress, std::string file);

        void workThreadMain();
        void onDispatcherNotification();
        void notifyThreadStopped();

        std::unique_ptr<std::thread> m_work_thread;
        Dispatcher m_dispatcher;
        ExportProgressInfoQueueSafe m_queue;
        uint64_t m_media_id;
        std::string m_iso_filepath;
        stream_callback m_stream_callback;
        Gtk::TreeView* m_export_view;
        Gtk::ListStore* m_export_store;
        Gtk::ProgressBar* m_progress_bar;
        Gtk::Button* m_ok_btn;
};


#endif //COLD_ARC_GTK_EXPORTDIALOG_H
