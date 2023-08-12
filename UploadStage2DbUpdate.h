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

using Stag2NotificationQueue = std::list<std::pair<bool, uint64_t>>;
using Stage2NotificationQueueSafe = BodyGuard<Stag2NotificationQueue>;

class UploadStage2DbUpdate {
    public:
        using sig_proto = sigc::slot<void(uint64_t, uint64_t, bool)>;
        UploadStage2DbUpdate(std::vector<UploadFileInfo>&& files, uint64_t parentId);
        void signal_update_notification(sig_proto&& slot);
        void start();
        uint64_t calculateTotalSize() const;
    private:
        void stage2Main();
        void onDispatcherNotification();
        void notifyThreadStopped();

        Stage2NotificationQueueSafe m_queue;
        std::vector<UploadFileInfo> m_files;
        std::unique_ptr<std::thread> m_stage2_thread;
        Dispatcher m_dispatcher;
        sigc::signal<void(uint64_t, uint64_t, bool)> m_gui_slot;
        uint64_t m_parentId;
};


#endif //COLD_ARC_GTK_UPLOADSTAGE2DBUPDATE_H
