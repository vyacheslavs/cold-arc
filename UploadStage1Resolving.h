//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADSTAGE1RESOLVING_H
#define COLD_ARC_GTK_UPLOADSTAGE1RESOLVING_H

#include <thread>
#include "UploadFilesCollection.h"
#include "Utils.h"
#include "UploadStage1Notification.h"
#include <chrono>
#include <optional>

using NotificationQueue = std::list<UploadStage1Notification>;
using NotificationQueueSafe = BodyGuard<NotificationQueue>;

class UploadStage1Resolving {
    public:
        explicit UploadStage1Resolving(UploadFilesCollection&& files);
        void signal_upload_notification(sigc::slot<void(const UploadStage1Notification&)>&& slot);
    private:
        enum class DispatcherEmitPolicy {
            Force,
            Throttled,
            NoEmit,
        };

        struct Dispatcher {
            sigc::connection connect(sigc::slot<void>&& slot);
            void emit(DispatcherEmitPolicy policy = DispatcherEmitPolicy::Force);
            bool timeToEmit() const;
            Glib::Dispatcher dispatcher;
            std::chrono::time_point<std::chrono::steady_clock> lastEmit;
        } m_dispatcher;

        std::unique_ptr<std::thread> m_stage1_thread;
        UploadFilesCollection m_files;
        NotificationQueueSafe m_stage1_notification_queue;
        std::set<std::string> m_root_folders;

        sigc::signal<void(const UploadStage1Notification&)> m_gui_slot;

        void stage1Main();
        void onDispatcherNotification();
        void enqueueNotification(UploadStage1Notification&& notification, DispatcherEmitPolicy policy = DispatcherEmitPolicy::Force);
        std::string guessFolder(const std::string& path);

    void processRegularFile(const Glib::RefPtr<Gio::File>& file, uint64_t fraction, uint64_t total);
};


#endif //COLD_ARC_GTK_UPLOADSTAGE1RESOLVING_H
