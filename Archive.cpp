//
// Created by developer on 8/6/23.
//

#include <iostream>
#include "Archive.h"

namespace arc {
    Archive &Archive::instance() {
        static Archive _ins;
        return _ins;
    }

    void Archive::newArchive(const Glib::ustring &filename) {
        auto endswith = [](const Glib::ustring &filename, const Glib::ustring &postfix) {
            if (postfix.size() > filename.size()) return false;
            return std::equal(postfix.rbegin(), postfix.rend(), filename.rbegin());
        };
        const auto fn = endswith(filename, ".db") ? filename : filename+".db";

        std::cout << "creating database at: "<<fn<<"\n";
        m_dbhandle = std::make_unique<SqLiteHandle>(filename);


    }

    Archive::~Archive() {
    }

    Archive::SqLiteHandle::SqLiteHandle(const Glib::ustring &filename) {
        auto rc = sqlite3_open(filename.c_str(), &m_db);
        if (rc)
            throw std::runtime_error(Glib::ustring::compose("failed to open database: %1", filename));
    }

    Archive::SqLiteHandle::~SqLiteHandle() {
        if (m_db)
            sqlite3_close(m_db);
    }
} // arc
