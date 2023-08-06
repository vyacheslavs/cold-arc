//
// Created by developer on 8/6/23.
//

#ifndef COLD_ARC_GTK_ARCHIVE_H
#define COLD_ARC_GTK_ARCHIVE_H

#include <glibmm/ustring.h>
#include <memory>
#include "sqlite3.h"

namespace arc {

    class Archive {
        private:
            Archive() = default;
            ~Archive();
        public:
            static Archive& instance();
            void newArchive(const Glib::ustring& filename);

        private:
            class SqLiteHandle {
            public:

                SqLiteHandle(const Glib::ustring& filename);
                ~SqLiteHandle();
            private:

                sqlite3* m_db;
                friend class Archive;
            };

            std::unique_ptr<SqLiteHandle> m_dbhandle;
    };

} // arc

#endif //COLD_ARC_GTK_ARCHIVE_H
