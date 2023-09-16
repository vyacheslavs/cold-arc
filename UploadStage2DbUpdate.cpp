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
    arc::Archive::ArchivePtr m_db;

    cold_arc::Error scope_error;
    ScopeExit scope([&] {
        if (scope_error.code == cold_arc::ErrorCode::None) {
            notifyThreadStopped();
            return;
        }
        notifyThreadStopped(scope_error);
    });

    auto dbres = arc::Archive::clone();
    if (!dbres) {
        scope_error = dbres.error();
        return;
    }
    m_db = std::move(dbres.value());
    if (auto res = m_db->beginTransaction(); !res ) {
        scope_error = res.error();
        return;
    }

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
        if (!file_folder) {
            auto rb = m_db->rollbackTransaction();
            scope_error = make_combined_error(file_folder.error(), rb.error(), cold_arc::ErrorCode::UploadThreadError);
            return;
        }
        auto resFile = m_db->createFile(item.getBasename(), item, file_folder.value());
        if (!resFile) {
            auto rb = m_db->rollbackTransaction();
            scope_error = make_combined_error(resFile.error(), rb.error(), cold_arc::ErrorCode::UploadThreadError);
            return;
        }
        if (!resFile.value()) {
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

    if (auto res = m_db->settings->media()->occupy(to_upload); !res) {
        auto rb = m_db->rollbackTransaction();
        scope_error = make_combined_error(res.error(), rb.error(), cold_arc::ErrorCode::UploadThreadError);
        return;
    }

    if (auto rb = m_db->commitTransaction(); !rb)
        scope_error = rb.error();
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
