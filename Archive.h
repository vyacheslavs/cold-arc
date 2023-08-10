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
            void createFolder(const Glib::ustring &name, uint64_t parentId = 1);
            uint64_t createPath(const Glib::ustring& path, uint64_t parentId = 1);

            template<typename F>
            void walkTree(F callback) {
                walkTree(callback, 0);
            }

            template<typename F>
            void walkTree(F callback, uint64_t parent_id) {
                if (!m_dbhandle)
                    return;
                m_dbhandle->select(Glib::ustring::compose("SELECT id, typ, name, hash, lnk, dt FROM arc_tree WHERE parent_id=%1", parent_id),
                                   [&](sqlite3_uint64 id, const char* typ, const char* name, const char* hash, const char* lnk, sqlite3_uint64 dt) {
                    callback(id, typ, name, hash, lnk, dt, parent_id);
                    walkTree(callback, id);
                });
            }

            template<typename F>
            void walkRoot(F callback, uint64_t id) {
                callback(id);
                m_dbhandle->select(Glib::ustring::compose("select parent_id FROM arc_tree WHERE id=%1", id), [&](sqlite3_uint64 parent_id) {
                    if (parent_id != 0)
                        walkRoot(callback, parent_id);
                });
            }

            [[nodiscard]] bool hasActiveArchive() const;
            [[nodiscard]] bool hasCurrentMedia() const;

        private:

            class Media {
                public:
                    Media() = default;
                    Media(Glib::ustring name, Glib::ustring serial, uint64_t cap) :
                        m_name(std::move(name)), m_serial(std::move(serial)), m_capacity(cap) {}

                    Media(Glib::ustring name, Glib::ustring serial, uint64_t cap, uint64_t occ, uint64_t loc) :
                        m_name(std::move(name)), m_serial(std::move(serial)), m_capacity(cap), m_occupied(occ), m_locked(loc) {}

                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] const Glib::ustring& serial() const;

                private:
                    Glib::ustring m_name;
                    Glib::ustring m_serial;
                    uint64_t m_capacity {0};
                    uint64_t m_occupied {0};
                    bool m_locked {false};

                friend class Archive;
            };

            class Settings {
                public:
                    explicit Settings(std::unique_ptr<SqLiteHandle>& _settings);
                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] const std::unique_ptr<Media>& media() const;
                    void updateName(const Glib::ustring& name);
                    void updateCurrentMedia(sqlite3_uint64 id);

                private:
                    Glib::ustring m_name;
                    sqlite3_uint64 m_currentMediaId {0};
                    std::unique_ptr<SqLiteHandle>& m_dbhandle;
                    std::unique_ptr<Media> m_currentMedia;

                    friend class Archive;
            };

            std::unique_ptr<SqLiteHandle> m_dbhandle;

    public:
            std::unique_ptr<Settings> settings;

    };


} // arc

#endif //COLD_ARC_GTK_ARCHIVE_H
