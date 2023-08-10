//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADSTAGE1RESOLVING_H
#define COLD_ARC_GTK_UPLOADSTAGE1RESOLVING_H

#include <thread>
#include "UploadFilesCollection.h"
#include "Utils.h"
#include "UploadFileInfo.h"
#include <chrono>
#include <optional>
#include "Dispatcher.h"

using NotificationQueue = std::list<UploadFileInfo>;
using NotificationQueueSafe = BodyGuard<NotificationQueue>;

class UploadStage1Resolving {
    public:
        explicit UploadStage1Resolving(UploadFilesCollection&& files);
        void signal_upload_notification(sigc::slot<void(const UploadFileInfo&)>&& slot);
    private:
        std::unique_ptr<std::thread> m_stage1_thread;
        UploadFilesCollection m_files;
        NotificationQueueSafe m_stage1_notification_queue;
        std::set<std::string> m_root_folders;
        sigc::signal<void(const UploadFileInfo&)> m_gui_slot;
        Dispatcher m_dispatcher;

        void stage1Main();
        void onDispatcherNotification();
        void enqueueNotification(UploadFileInfo&& notification, DispatcherEmitPolicy policy = DispatcherEmitPolicy::Force);
        std::string guessFolder(const std::string& path);

    void processRegularFile(const Glib::RefPtr<Gio::File>& file, uint64_t fraction, uint64_t total);
};


#endif //COLD_ARC_GTK_UPLOADSTAGE1RESOLVING_H
