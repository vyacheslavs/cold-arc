//
// Created by developer on 8/10/23.
//

#include "UploadStage2DbUpdate.h"
#include "Archive.h"

UploadStage2DbUpdate::UploadStage2DbUpdate(std::vector<UploadFileInfo>&& files, uint64_t parentId)
    : m_files(std::move(files)),
      m_parentId(parentId) {
    m_dispatcher.connect(sigc::mem_fun(this, &UploadStage2DbUpdate::onDispatcherNotification));
}

void UploadStage2DbUpdate::stage2Main(uint64_t cap_limit) {

    std::unique_ptr<arc::Archive> m_db;
    try {
        m_db = arc::Archive::clone();
    } catch (const sqlite::sqlite_exception& e) {
        notifyThreadStopped(e);
        return;
    } catch (const WrongDatabaseVersion& e) {
        notifyThreadStopped(e);
        return;
    }

    auto p = m_db->savePoint();
    try {
        uint64_t to_upload = 0;
        for (const auto& item: m_files) {
            if (item.isSkipped())
                continue;

            to_upload += item.getSize();
            if (to_upload > cap_limit) {
                to_upload -= item.getSize();
                break;
            }

            auto file_folder = m_db->createPath(item.getFolder(), m_parentId, true);
            if (!m_db->createFile(item.getBasename(), item, file_folder)) {
                to_upload -= item.getSize();
            }

            if (m_dispatcher.timeToEmit()) {
                {
                    auto cc = m_queue.access();
                    cc->emplace_back(true, item.index);
                }
                m_dispatcher.emit(DispatcherEmitPolicy::Throttled);
            }
        }

        m_db->settings->media()->occupy(to_upload);
        notifyThreadStopped();
    } catch (const sqlite::sqlite_exception& e) {
        std::cout << "rolled back!\n";
        p.rollback();
        notifyThreadStopped(e);
    }
}

void UploadStage2DbUpdate::onDispatcherNotification() {
    Stage2NotificationQueue q;
    {
        auto cc = m_queue.access();
        q = std::move(*cc);
    }
    for (const auto& item: q) {

        m_gui_slot.emit(item.index, m_files.size() + 1, !item.thread_running, item.e);

        if (!item.thread_running) {
            m_stage2_thread->join();
            m_stage2_thread.reset();
            break;
        }
    }
}

void UploadStage2DbUpdate::signal_update_notification(UploadStage2DbUpdate::sig_proto&& slot) {
    m_gui_slot.connect(std::move(slot));
}

void UploadStage2DbUpdate::start(uint64_t cap_limit) {
    m_stage2_thread = std::make_unique<std::thread>(&UploadStage2DbUpdate::stage2Main, this, cap_limit);
}

void UploadStage2DbUpdate::notifyThreadStopped() {
    {
        auto acc = m_queue.access();
        acc->emplace_back(false, 0);
    }
    m_dispatcher.emit();
}
