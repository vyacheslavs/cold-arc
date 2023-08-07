//
// Created by developer on 8/7/23.
//

#include "Utils.h"
#include <sys/stat.h>
#include <random>

bool endsWith(const Glib::ustring &filename, const Glib::ustring &postfix) {
    if (postfix.size() > filename.size()) return false;
    return std::equal(postfix.rbegin(), postfix.rend(), filename.rbegin());
}

bool fileExists(const Glib::ustring &filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0;
}

Glib::ustring generateSerial() {
    auto length = 16;
    static auto& chrs = "0123456789"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(length);

    while(length--)
        s += chrs[pick(rg)];

    return s;
}
