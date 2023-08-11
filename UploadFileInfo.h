//
// Created by developer on 8/9/23.
//

#ifndef COLD_ARC_GTK_UPLOADFILEINFO_H
#define COLD_ARC_GTK_UPLOADFILEINFO_H

#include <string>

enum class UploadFileStatus {
    UNKNOWN,
    HASHING,
    PROCESSED,
    FAILED_TO_OPEN,
    FAILED_TO_HASH,
    SKIPPED,
};

class UploadFileInfo {
public:
    [[nodiscard]] bool isThreadStopped() const;

    [[nodiscard]] bool isProcessed() const;

    [[nodiscard]] bool isSkipped() const;

    [[nodiscard]] bool isHashing() const;

    [[nodiscard]] bool isFailedToOpen() const;

    [[nodiscard]] bool isFailedToHash() const;

    [[nodiscard]] const std::string& getPath() const;

    [[nodiscard]] const std::string& getBasename() const;

    [[nodiscard]] const std::string& getFolder() const;

    [[nodiscard]] const std::string& getHash() const;

    [[nodiscard]] uint32_t getMode() const;

    [[nodiscard]] uint64_t getMtime() const;

    [[nodiscard]] uint64_t getSize() const;

    [[nodiscard]] static UploadFileInfo threadStoppedNotification();

    [[nodiscard]] static UploadFileInfo skipped(const std::string& path);

    [[nodiscard]] static UploadFileInfo failedToOpen(const std::string& path);

    [[nodiscard]] static UploadFileInfo failedToHash(const std::string& path);

    [[nodiscard]] static UploadFileInfo
    hashing(uint64_t fraction, uint64_t total, const std::string& basename);

    [[nodiscard]] static UploadFileInfo
    processed(uint64_t fraction, uint64_t total, const std::string& path,
              const std::string& basename, uint64_t size_in_bytes, uint64_t mtime,
              const std::string& hash, const std::string& folder, uint32_t mode);

    [[nodiscard]] double fraction() const;

    void skip();

    uint64_t index{0};
private:
    UploadFileInfo() = default;

    bool m_thread_stopped{false};
    UploadFileStatus m_status{UploadFileStatus::UNKNOWN};
    std::string m_path;
    std::string m_basename;
    std::string m_hash;
    std::string m_hiera;
    uint64_t m_progress_total{0};
    uint64_t m_progress_fraction{0};
    uint64_t m_size_in_bytes{0};
    uint64_t m_mtime{0};
    uint32_t m_mode{0};
};


#endif //COLD_ARC_GTK_UPLOADFILEINFO_H
