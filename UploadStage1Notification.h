//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADSTAGE1NOTIFICATION_H
#define COLD_ARC_GTK_UPLOADSTAGE1NOTIFICATION_H

#include <string>

enum class UploadStage1FileStatus {
    UNKNOWN,
    HASHING,
    PROCESSED,
    FAILED_TO_OPEN,
    FAILED_TO_HASH,
    SKIPPED,
};

class UploadStage1Notification {
    public:
        [[nodiscard]] bool isThreadStopped() const;
        [[nodiscard]] bool isProcessed() const;
        [[nodiscard]] bool isSkipped() const;
        [[nodiscard]] bool isHashing() const;
        [[nodiscard]] bool isFailedToOpen() const;
        [[nodiscard]] bool isFailedToHash() const;
        [[nodiscard]] const std::string& getPath() const;
        [[nodiscard]] const std::string& getBasename() const;
        [[nodiscard]] static UploadStage1Notification threadStoppedNotification();
        [[nodiscard]] static UploadStage1Notification skipped(const std::string& path);
        [[nodiscard]] static UploadStage1Notification failedToOpen(const std::string& path);
        [[nodiscard]] static UploadStage1Notification failedToHash(const std::string& path);
        [[nodiscard]] static UploadStage1Notification hashing(uint64_t fraction, uint64_t total, const std::string& basename);
        [[nodiscard]] static UploadStage1Notification
        processed(uint64_t fraction, uint64_t total, const std::string& path,
                  const std::string& basename, uint64_t size_in_bytes, uint64_t mtime,
                  const std::string& hash);
        [[nodiscard]] double fraction() const;
    private:
        UploadStage1Notification() = default;
        bool m_thread_stopped {false};
        UploadStage1FileStatus m_status {UploadStage1FileStatus::UNKNOWN};
        std::string m_path;
        std::string m_basename;
        std::string m_hash;
        uint64_t m_progress_total {0};
        uint64_t m_progress_fraction {0};
        uint64_t m_size_in_bytes {0};
        uint64_t m_mtime{0};

};


#endif //COLD_ARC_GTK_UPLOADSTAGE1NOTIFICATION_H
