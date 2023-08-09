//
// Created by developer on 8/9/23.
//

#include "UploadStage1Resolving.h"
#include <memory>
#include <iostream>

UploadStage1Resolving::UploadStage1Resolving(UploadFilesCollection&& files) : m_files(std::move(files)) {
    m_dispatcher.connect(sigc::mem_fun(this, &UploadStage1Resolving::onDispatcherNotification));
    m_stage1_thread = std::make_unique<std::thread>(&UploadStage1Resolving::stage1Main, this);
}

void UploadStage1Resolving::stage1Main() {
    uint64_t index = 0;
    for (const auto& item: m_files) {
        auto file_type = item->query_file_type(Gio::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);
        if (file_type == Gio::FILE_TYPE_REGULAR)
            processRegularFile(item, index++, m_files.size());
        else {
            enqueueNotification(UploadStage1Notification::skipped(item->get_path()));
        }
    }

    // send that thread is stopped
    enqueueNotification(UploadStage1Notification::threadStoppedNotification());
}

void UploadStage1Resolving::onDispatcherNotification() {
    NotificationQueue q;
    {
        auto acc = m_stage1_notification_queue.access();
        q = std::move(*acc);
    }
    bool threadStopped = false;
    for (const auto& notification: q) {
        m_gui_slot.emit(notification);
        if (notification.isThreadStopped())
            threadStopped = true;
    }
    if (threadStopped) {
        m_stage1_thread->join();
        m_stage1_thread.reset();
    }
}

void UploadStage1Resolving::processRegularFile(const Glib::RefPtr<Gio::File>& file, uint64_t fraction, uint64_t total) {
    struct stat st{};
    if (stat(file->get_path().c_str(), &st) < 0) {
        enqueueNotification(UploadStage1Notification::failedToOpen(file->get_path()));
        return;
    }
    uint64_t size_in_bytes = st.st_size;
    uint64_t dt_mtime = st.st_mtim.tv_sec;
    auto res = calculateSha256(file->get_path(), size_in_bytes, [&](uint64_t fraction) {
        enqueueNotification(UploadStage1Notification::hashing(fraction, size_in_bytes, file->get_path()),
                            DispatcherEmitPolicy::Throttled);
    });
    if (!res) {
        enqueueNotification(UploadStage1Notification::failedToHash(file->get_path()));
        return;
    }
    enqueueNotification(
            UploadStage1Notification::processed(fraction, total, file->get_path(), file->get_basename(), size_in_bytes,
                                                dt_mtime,
                                                res.value()));
}

void UploadStage1Resolving::enqueueNotification(UploadStage1Notification&& notification, DispatcherEmitPolicy policy) {
    {
        auto acc = m_stage1_notification_queue.access();
        acc->push_back(std::move(notification));
    }
    m_dispatcher.emit(policy);
}

void UploadStage1Resolving::signal_upload_notification(sigc::slot<void(const UploadStage1Notification&)>&& slot) {
    m_gui_slot.connect(std::move(slot));
}

sigc::connection UploadStage1Resolving::Dispatcher::connect(sigc::slot<void>&& slot) {
    return dispatcher.connect(std::move(slot));
}

void UploadStage1Resolving::Dispatcher::emit(UploadStage1Resolving::DispatcherEmitPolicy policy) {
    if (policy == DispatcherEmitPolicy::Force)
        dispatcher.emit();
    else if (policy == DispatcherEmitPolicy::Throttled) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastEmit).count() > 0) {
            lastEmit = now;
            dispatcher.emit();
        }
    }
}

