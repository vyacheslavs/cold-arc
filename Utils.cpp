//
// Created by developer on 8/7/23.
//

#include "Utils.h"
#include <sys/stat.h>

bool endsWith(const Glib::ustring &filename, const Glib::ustring &postfix) {
    if (postfix.size() > filename.size()) return false;
    return std::equal(postfix.rbegin(), postfix.rend(), filename.rbegin());
}

bool fileExists(const Glib::ustring &filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0;
}
