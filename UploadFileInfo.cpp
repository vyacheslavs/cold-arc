//
// Created by developer on 8/9/23.
//

#include "UploadFileInfo.h"

bool UploadFileInfo::isThreadStopped() const {
    return m_thread_stopped;
}

UploadFileInfo UploadFileInfo::threadStoppedNotification() {
    UploadFileInfo ret;
    ret.m_thread_stopped = true;
    return ret;
}

UploadFileInfo UploadFileInfo::failedToOpen(const std::string& path) {
    UploadFileInfo ret;
    ret.m_path = path;
    ret.m_status = UploadFileStatus::FAILED_TO_OPEN;
    return ret;
}

UploadFileInfo UploadFileInfo::hashing(uint64_t fraction, uint64_t total, const std::string& basename) {
    UploadFileInfo ret;
    ret.m_basename = basename;
    ret.m_status = UploadFileStatus::HASHING;
    ret.m_progress_total = total;
    ret.m_progress_fraction = fraction;
    return ret;
}

UploadFileInfo UploadFileInfo::failedToHash(const std::string& path) {
    UploadFileInfo ret;
    ret.m_path = path;
    ret.m_status = UploadFileStatus::FAILED_TO_HASH;
    return ret;
}

UploadFileInfo
UploadFileInfo::processed(uint64_t fraction, uint64_t total, const std::string& path,
                                    const std::string& basename, uint64_t size_in_bytes, uint64_t mtime,
                                    const std::string& hash, const std::string& folder) {
    UploadFileInfo ret;
    ret.m_path = path;
    ret.m_basename = basename;
    ret.m_progress_fraction = fraction;
    ret.m_progress_total = total;
    ret.m_size_in_bytes = size_in_bytes;
    ret.m_mtime = mtime;
    ret.m_hash = hash;
    ret.m_hiera = folder;
    ret.m_status = UploadFileStatus::PROCESSED;
    return ret;
}

bool UploadFileInfo::isProcessed() const {
    return m_status == UploadFileStatus::PROCESSED;
}

const std::string& UploadFileInfo::getPath() const {
    return m_path;
}

const std::string& UploadFileInfo::getBasename() const {
    return m_basename;
}

UploadFileInfo UploadFileInfo::skipped(const std::string& path) {
    UploadFileInfo ret;
    ret.m_path = path;
    ret.m_status = UploadFileStatus::SKIPPED;
    return ret;
}

bool UploadFileInfo::isSkipped() const {
    return m_status == UploadFileStatus::SKIPPED;
}

#include <iostream>

double UploadFileInfo::fraction() const {
    if (!m_progress_total)
        return 0;
    return static_cast<double>(m_progress_fraction) / static_cast<double>(m_progress_total);
}

bool UploadFileInfo::isHashing() const {
    return m_status == UploadFileStatus::HASHING;
}

bool UploadFileInfo::isFailedToHash() const {
    return m_status == UploadFileStatus::FAILED_TO_HASH;
}

bool UploadFileInfo::isFailedToOpen() const {
    return m_status == UploadFileStatus::FAILED_TO_OPEN;
}

void UploadFileInfo::skip() {
    m_status = UploadFileStatus::SKIPPED;
}

const std::string& UploadFileInfo::getFolder() const {
    return m_hiera;
}

const std::string& UploadFileInfo::getHash() const {
    return m_hash;
}

uint64_t UploadFileInfo::getSize() const {
    return m_size_in_bytes;
}

uint64_t UploadFileInfo::getMtime() const {
    return m_mtime;
}
