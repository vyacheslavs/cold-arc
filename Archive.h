//
// Created by developer on 8/6/23.
//

#ifndef COLD_ARC_GTK_ARCHIVE_H
#define COLD_ARC_GTK_ARCHIVE_H

#include <glibmm/ustring.h>
#include <memory>
#include <optional>
#include "SqLiteHandle.h"

namespace arc {

    class Archive {
        private:
            Archive() = default;
        public:
            static Archive& instance();
            void newArchive(const Glib::ustring& filename);
            void openArchive(const Glib::ustring& filename);
            void newMedia(const Glib::ustring& name, const Glib::ustring& serial, int capacity);

            bool hasActiveArchive() const;

        private:

            class Media {
                public:

            };

            class Settings {
                public:
                    explicit Settings(std::unique_ptr<SqLiteHandle>& _settings);
                    [[nodiscard]] const Glib::ustring& name() const;
                    void updateName(const Glib::ustring& name);
                    void updateCurrentMedia(sqlite3_uint64 id);

                private:
                    Glib::ustring m_name;
                    std::optional<sqlite3_uint64> m_current_media_id;
                    std::unique_ptr<SqLiteHandle>& m_dbhandle;
                    std::unique_ptr<Media> m_currentMedia;
            };

            std::unique_ptr<SqLiteHandle> m_dbhandle;

    public:
            std::unique_ptr<Settings> settings;

    };


} // arc

#endif //COLD_ARC_GTK_ARCHIVE_H
