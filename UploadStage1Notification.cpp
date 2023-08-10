//
// Created by developer on 8/9/23.
//

#include "UploadStage1Notification.h"

bool UploadStage1Notification::isThreadStopped() const {
    return m_thread_stopped;
}

UploadStage1Notification UploadStage1Notification::threadStoppedNotification() {
    UploadStage1Notification ret;
    ret.m_thread_stopped = true;
    return ret;
}

UploadStage1Notification UploadStage1Notification::failedToOpen(const std::string& path) {
    UploadStage1Notification ret;
    ret.m_path = path;
    ret.m_status = UploadStage1FileStatus::FAILED_TO_OPEN;
    return ret;
}

UploadStage1Notification UploadStage1Notification::hashing(uint64_t fraction, uint64_t total, const std::string& basename) {
    UploadStage1Notification ret;
    ret.m_basename = basename;
    ret.m_status = UploadStage1FileStatus::HASHING;
    ret.m_progress_total = total;
    ret.m_progress_fraction = fraction;
    return ret;
}

UploadStage1Notification UploadStage1Notification::failedToHash(const std::string& path) {
    UploadStage1Notification ret;
    ret.m_path = path;
    ret.m_status = UploadStage1FileStatus::FAILED_TO_HASH;
    return ret;
}

UploadStage1Notification
UploadStage1Notification::processed(uint64_t fraction, uint64_t total, const std::string& path,
                                    const std::string& basename, uint64_t size_in_bytes, uint64_t mtime,
                                    const std::string& hash, const std::string& folder) {
    UploadStage1Notification ret;
    ret.m_path = path;
    ret.m_basename = basename;
    ret.m_progress_fraction = fraction;
    ret.m_progress_total = total;
    ret.m_size_in_bytes = size_in_bytes;
    ret.m_mtime = mtime;
    ret.m_hash = hash;
    ret.m_hiera = folder;
    ret.m_status = UploadStage1FileStatus::PROCESSED;
    return ret;
}

bool UploadStage1Notification::isProcessed() const {
    return m_status == UploadStage1FileStatus::PROCESSED;
}

const std::string& UploadStage1Notification::getPath() const {
    return m_path;
}

const std::string& UploadStage1Notification::getBasename() const {
    return m_basename;
}

UploadStage1Notification UploadStage1Notification::skipped(const std::string& path) {
    UploadStage1Notification ret;
    ret.m_path = path;
    ret.m_status = UploadStage1FileStatus::SKIPPED;
    return ret;
}

bool UploadStage1Notification::isSkipped() const {
    return m_status == UploadStage1FileStatus::SKIPPED;
}

#include <iostream>

double UploadStage1Notification::fraction() const {
    if (!m_progress_total)
        return 0;
    return static_cast<double>(m_progress_fraction) / static_cast<double>(m_progress_total);
}

bool UploadStage1Notification::isHashing() const {
    return m_status == UploadStage1FileStatus::HASHING;
}

bool UploadStage1Notification::isFailedToHash() const {
    return m_status == UploadStage1FileStatus::FAILED_TO_HASH;
}

bool UploadStage1Notification::isFailedToOpen() const {
    return m_status == UploadStage1FileStatus::FAILED_TO_OPEN;
}

void UploadStage1Notification::skip() {
    m_status = UploadStage1FileStatus::SKIPPED;
}

const std::string& UploadStage1Notification::getFolder() const {
    return m_hiera;
}

const std::string& UploadStage1Notification::getHash() const {
    return m_hash;
}

uint64_t UploadStage1Notification::getSize() const {
    return m_size_in_bytes;
}
