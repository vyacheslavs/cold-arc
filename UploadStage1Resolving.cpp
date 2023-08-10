//
// Created by developer on 8/9/23.
//

#include "UploadStage1Resolving.h"
#include <memory>
#include <filesystem>
#include <iostream>

UploadStage1Resolving::UploadStage1Resolving(UploadFilesCollection&& files) : m_files(std::move(files)) {
    m_dispatcher.connect(sigc::mem_fun(this, &UploadStage1Resolving::onDispatcherNotification));
    m_stage1_thread = std::make_unique<std::thread>(&UploadStage1Resolving::stage1Main, this);
}

void UploadStage1Resolving::stage1Main() {
    uint64_t index = 0;

    for (; index < m_files.size(); ++index) {
        auto& item = m_files[index];
        auto file_type = item->query_file_type(Gio::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);

        if (file_type == Gio::FILE_TYPE_REGULAR) {
            processRegularFile(item, index, m_files.size());
        } else if (file_type == Gio::FILE_TYPE_DIRECTORY) {
            // list this directory and add them to m_files
            m_root_folders.insert(std::filesystem::path(item->get_path()).parent_path());
            for (const auto& sub : std::filesystem::directory_iterator(item->get_path())) {
                m_files.push_back(Gio::File::create_for_path(sub.path()));
            }
        } else {
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
        enqueueNotification(UploadStage1Notification::hashing(fraction, size_in_bytes, file->get_basename()),
                            DispatcherEmitPolicy::Throttled);
    });
    if (!res) {
        enqueueNotification(UploadStage1Notification::failedToHash(file->get_path()));
        return;
    }

    enqueueNotification(
            UploadStage1Notification::processed(fraction, total, file->get_path(), file->get_basename(), size_in_bytes,
                                                dt_mtime,
                                                res.value(), guessFolder(file->get_path())));
}

void UploadStage1Resolving::enqueueNotification(UploadStage1Notification&& notification, DispatcherEmitPolicy policy) {
    if (m_dispatcher.timeToEmit() || policy == DispatcherEmitPolicy::Force){
        auto acc = m_stage1_notification_queue.access();
        acc->push_back(std::move(notification));
    }
    m_dispatcher.emit(policy);
}

void UploadStage1Resolving::signal_upload_notification(sigc::slot<void(const UploadStage1Notification&)>&& slot) {
    m_gui_slot.connect(std::move(slot));
}

std::string UploadStage1Resolving::guessFolder(const std::string& path) {

    std::string folder = std::filesystem::path(path).parent_path();
    bool root_folder_found = false;
    for (const auto& root_folder : m_root_folders) {
        if (folder.rfind(root_folder, 0) == 0) {
            folder = folder.substr(root_folder.length());
            root_folder_found = true;
            break;
        }
    }
    if (!root_folder_found) {
        m_root_folders.insert(folder);
        folder = "";
    }
    return folder;
}

sigc::connection UploadStage1Resolving::Dispatcher::connect(sigc::slot<void>&& slot) {
    return dispatcher.connect(std::move(slot));
}

void UploadStage1Resolving::Dispatcher::emit(UploadStage1Resolving::DispatcherEmitPolicy policy) {
    if (policy == DispatcherEmitPolicy::Force)
        dispatcher.emit();
    else if (policy == DispatcherEmitPolicy::Throttled) {
        if (timeToEmit()) {
            lastEmit = std::chrono::steady_clock::now();
            dispatcher.emit();
        }
    }
}

bool UploadStage1Resolving::Dispatcher::timeToEmit() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - lastEmit).count() > 0;
}

