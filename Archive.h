//
// Created by developer on 8/6/23.
//

#ifndef COLD_ARC_GTK_ARCHIVE_H
#define COLD_ARC_GTK_ARCHIVE_H

#include <glibmm/ustring.h>
#include <memory>
#include <optional>
#include <cstdint>
#include "sqlite_modern_cpp.h"
#include "UploadFileInfo.h"
#include "Utils.h"

namespace arc {

    class Archive {
        private:
            Archive() = default;
        public:
            static Archive& instance();
            [[nodiscard]] static std::unique_ptr<Archive> clone();
            void newArchive(const Glib::ustring& filename);
            void openArchive(const Glib::ustring& filename);
            void newMedia(const Glib::ustring& name, const Glib::ustring& serial, int capacity);
            void init(const Glib::ustring& filename);
            uint64_t createFolder(const Glib::ustring& name, uint64_t parentId = 1, bool quiet = false);
            uint64_t createPath(const Glib::ustring& path, uint64_t parentId = 1, bool quiet = false);
            uint64_t createFile(const Glib::ustring& name, const UploadFileInfo& file_info, uint64_t parentId = 1);

            template<typename F>
            void walkTree(F callback, bool folders_only = false) {
                walkTree(callback, 0, folders_only);
            }

            template<typename F>
            void walkTree(F callback, uint64_t parent_id, bool folders_only = false) {
                try {
                    *m_dbhandle
                        << (folders_only ? "SELECT id, typ, name, hash, lnk, dt FROM arc_tree WHERE parent_id=? and typ='folder'" : "SELECT id, typ, name, hash, lnk, dt FROM arc_tree WHERE parent_id=?")
                        << parent_id
                        >> [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk, sqlite3_uint64 dt) {
                            callback(id, typ, name, hash, lnk, dt, parent_id);
                            walkTree(callback, id, folders_only);
                        };
                } catch (const std::runtime_error& e) {
                    assert_fail(e);
                }
            }

            template<typename F>
            void browse(F callback, uint64_t parent_id) {
                try {
                    *m_dbhandle
                        << "SELECT id, typ, name, siz, hash FROM arc_tree WHERE parent_id=? ORDER BY typ DESC"
                        << parent_id
                        >> callback;
                } catch (const std::exception& e) {
                    assert_fail(e);
                }
            }

            template<typename F>
            void browseMedia(F callback) {
                try {
                    *m_dbhandle
                        << "SELECT id, capacity, name, serial FROM arc_media"
                        >> callback;
                } catch (const std::exception& e) {
                    assert_fail(e);
                }
            }

            template<typename F>
            void walkRoot(F callback, uint64_t id) {
                callback(id);
                try {
                    *m_dbhandle
                        << "SELECT parent_id FROM arc_tree WHERE id=?"
                        << id
                        >> [&](sqlite3_uint64 parent_id) {
                            if (parent_id != 0)
                                walkRoot(callback, parent_id);
                        };
                } catch (const std::runtime_error& e) {
                    assert_fail(e);
                }
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
                    explicit Settings(std::unique_ptr<sqlite::database>& _settings);
                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] sqlite3_uint64 mediaId() const;
                    [[nodiscard]] const std::unique_ptr<Media>& media() const;
                    void updateName(const Glib::ustring& name);
                    void updateCurrentMedia(sqlite3_uint64 id);

                private:
                    Glib::ustring m_name;
                    sqlite3_uint64 m_currentMediaId {0};
                    std::unique_ptr<sqlite::database>& m_dbhandle;
                    std::unique_ptr<Media> m_currentMedia;

                    friend class Archive;
            };

            std::string m_dbname;
            std::unique_ptr<sqlite::database> m_dbhandle;

    public:
            std::unique_ptr<Settings> settings;

    };


} // arc

#endif //COLD_ARC_GTK_ARCHIVE_H
