//
// Created by developer on 8/10/23.
//

#ifndef COLD_ARC_GTK_UPLOADSTAGE2DBUPDATE_H
#define COLD_ARC_GTK_UPLOADSTAGE2DBUPDATE_H

#include <gtkmm-3.0/gtkmm.h>
#include "Dispatcher.h"
#include "UploadFileInfo.h"
#include <thread>
#include "Utils.h"
#include "Archive.h"

struct Stage2Notification {

    Stage2Notification(bool _is_running, uint64_t idx) : thread_running(_is_running), index(idx) {}
    template <typename E>
    Stage2Notification(bool _is_running, uint64_t idx, const E& _e)
        : thread_running(_is_running), index(idx), e(_e) {}

    bool thread_running {false};
    uint64_t index {0};
    cold_arc::Error e;
};

using Stage2NotificationQueue = std::list<Stage2Notification>;
using Stage2NotificationQueueSafe = BodyGuard<Stage2NotificationQueue>;

class UploadStage2DbUpdate {
    public:
        using sig_proto = sigc::slot<void(uint64_t, uint64_t, bool, const cold_arc::Error&)>;
        UploadStage2DbUpdate(std::vector<UploadFileInfo>&& files, uint64_t parentId);
        void signal_update_notification(sig_proto&& slot);
        void start(uint64_t cap_limit);
    private:
        void stage2Main(uint64_t cap_limit);
        void onDispatcherNotification();
        void notifyThreadStopped();

        template<typename E>
        void notifyThreadStopped(E&& e) {
            {
                auto cc = m_queue.access();
                cc->emplace_back(false, 0, std::forward<E>(e));
            }
            m_dispatcher.emit(DispatcherEmitPolicy::Force);
        }

        Stage2NotificationQueueSafe m_queue;
        std::vector<UploadFileInfo> m_files;
        std::unique_ptr<std::thread> m_stage2_thread;
        Dispatcher m_dispatcher;
        sigc::signal<void(uint64_t, uint64_t, bool, const cold_arc::Error&)> m_gui_slot;
        uint64_t m_parentId;
};


#endif //COLD_ARC_GTK_UPLOADSTAGE2DBUPDATE_H
