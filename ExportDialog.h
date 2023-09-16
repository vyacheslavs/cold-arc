//
// Created by developer on 9/7/23.
//

#ifndef COLD_ARC_GTK_EXPORTDIALOG_H
#define COLD_ARC_GTK_EXPORTDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "ISOBuilder.h"
#include "Dispatcher.h"
#include "Utils.h"
#include <thread>

enum class progress_type {
        unknown,
        overall,
        last_file,
};

struct ExportProgressInfo {
    bool running {true};
    cold_arc::Error e;

    progress_type prog_type {progress_type::unknown};
    int progress {0}; // overall progress
    std::string file_name{0};

    explicit ExportProgressInfo(bool is_running) : running(is_running) {}
    explicit ExportProgressInfo(std::string fn, int prog) : running(true), prog_type(progress_type::last_file), progress(prog), file_name(std::move(fn)) {}
    ExportProgressInfo(int prog, std::string fn) : running(true), prog_type(progress_type::overall), progress(prog), file_name(std::move(fn)) {}

    template<typename E>
    explicit ExportProgressInfo(E && _e, bool is_running) : running(is_running), e(std::forward<E>(_e)) {}
};

using ExportProgressInfoQueue = std::list<ExportProgressInfo>;
using ExportProgressInfoQueueSafe = BodyGuard<ExportProgressInfoQueue>;

class ExportDialog : public Gtk::Dialog {
    public:
        ExportDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

        [[nodiscard]] static cold_arc::Result<> run(uint64_t media_id, const std::string& iso_file);
        [[nodiscard]] cold_arc::Result<> construct(const Glib::RefPtr<Gtk::Builder>& builder, uint64_t media_id, std::string iso_file);

    private:

        template<typename E>
        void notifyThreadStopped(E&& e) {
            {
                auto cc = m_queue.access();
                cc->emplace_back(std::forward<E>(e), false);
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
        uint64_t m_media_id {0};
        std::string m_iso_filepath;
        stream_callback m_stream_callback{nullptr};
        Gtk::TreeView* m_export_view {nullptr};
        Gtk::ListStore* m_export_store {nullptr};
        Gtk::ProgressBar* m_progress_bar {nullptr};
        Gtk::Button* m_ok_btn {nullptr};
};


#endif //COLD_ARC_GTK_EXPORTDIALOG_H
