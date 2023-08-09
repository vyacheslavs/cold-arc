//
// Created by developer on 8/8/23.
//

#ifndef COLD_ARC_GTK_PROGRESSINFO_H
#define COLD_ARC_GTK_PROGRESSINFO_H

#include <cstdint>
#include <string>

struct ProgressInfo {
    enum class Action {
        UNKNOWN,
        HASHING
    };
    ProgressInfo() = default;
    bool upload_in_progress {false};
    int percent {0};
    int total_percent {0};
    std::string info;
    Action action {Action::UNKNOWN};
};

struct UploadFileInfo {
    uint64_t sizeInBytes {0};
    uint64_t dtOriginal {0}; // original m_mtime
    std::string sha256hash;
};


#endif //COLD_ARC_GTK_PROGRESSINFO_H
