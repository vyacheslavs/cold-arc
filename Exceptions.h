//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_EXCEPTIONS_H
#define COLD_ARC_GTK_EXCEPTIONS_H

#include <stdexcept>
#include <glibmm/ustring.h>

class WrongDatabaseVersion : public std::runtime_error {
public:
    WrongDatabaseVersion(uint64_t vnow, uint64_t vexpected) : std::runtime_error(Glib::ustring::compose("Wrong databse version, supplied %1, expected %2", vnow, vexpected)) {}
};

#endif //COLD_ARC_GTK_EXCEPTIONS_H
