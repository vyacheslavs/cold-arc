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

void UploadStage2DbUpdate::stage2Main() {

    std::unique_ptr<arc::Archive> m_db(arc::Archive::clone());

    for (const auto& item: m_files) {
        if (item.isSkipped())
            continue;

        auto file_folder = m_db->createPath(item.getFolder(), m_parentId, true);
        m_db->createFile(item.getBasename(), item, file_folder);

        if (m_dispatcher.timeToEmit()) {
            {
                auto cc = m_queue.access();
                cc->emplace_back(true, item.index);
            }
            m_dispatcher.emit(DispatcherEmitPolicy::Throttled);
        }
    }

    notifyThreadStopped();
}

void UploadStage2DbUpdate::onDispatcherNotification() {
    Stag2NotificationQueue q;
    {
        auto cc = m_queue.access();
        q = std::move(*cc);
    }
    for (const auto& item: q) {

        m_gui_slot.emit(item.second, m_files.size() + 1, !item.first);

        if (!item.first) {
            m_stage2_thread->join();
            m_stage2_thread.reset();
            break;
        }
    }
}

void UploadStage2DbUpdate::signal_update_notification(UploadStage2DbUpdate::sig_proto&& slot) {
    m_gui_slot.connect(std::move(slot));
}

void UploadStage2DbUpdate::start() {
    m_stage2_thread = std::make_unique<std::thread>(&UploadStage2DbUpdate::stage2Main, this);
}

void UploadStage2DbUpdate::notifyThreadStopped() {
    {
        auto acc = m_queue.access();
        acc->emplace_back(false, 0);
    }
    m_dispatcher.emit();
}

uint64_t UploadStage2DbUpdate::calculateTotalSize() const {
    uint64_t total = 0;
    for (const auto& item: m_files) {
        if (item.isSkipped())
            continue;
        total += item.getSize();
    }
    return total;
}
