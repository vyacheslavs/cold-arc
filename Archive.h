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
            void newMedia(const Glib::ustring& name, const Glib::ustring& serial, uint64_t capacity);
            void init(const Glib::ustring& filename);
            uint64_t createFolder(const Glib::ustring& name, uint64_t parentId = 1, bool quiet = false);
            uint64_t createPath(const Glib::ustring& path, uint64_t parentId = 1, bool quiet = false);
            bool createFile(const Glib::ustring& name, const UploadFileInfo& file_info, uint64_t parentId = 1);

            template<typename F>
            void walkTree(F callback, uint64_t parent_id, const std::string& exclusions = std::string()) {
                try {
                    // select arc_tree.*, arc_tree_to_media.arc_media_id IN (1) as A from arc_tree inner join arc_tree_to_media on arc_tree.id=arc_tree_to_media.arc_tree_id where arc_tree.parent_id=1 and A=1;
                    auto lcall = [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk, sqlite3_uint64 dt) {
                        callback(id, typ, name, hash, lnk, dt, parent_id);
                        walkTree(callback, id, exclusions);
                    };

                    if (exclusions.empty()) {
                        *m_dbhandle
                            << "SELECT id, typ, name, hash, lnk, dt FROM arc_tree WHERE parent_id=? and typ='folder'"
                            << parent_id
                            >> lcall;
                            ;
                    } else {
                        *m_dbhandle
                            << Glib::ustring::compose("SELECT id, typ, name, hash, lnk, dt, arc_media_id IN (%1) as A FROM arc_tree INNER JOIN arc_tree_to_media on id=arc_tree_id WHERE arc_tree.parent_id=? and A=1 AND typ='folder' GROUP BY id", exclusions).operator std::string()
                            << parent_id
                            >> lcall;
                    }
                } catch (const std::runtime_error& e) {
                    assert_fail(e);
                }
            }

            template<typename F>
            void browse(F callback, uint64_t parent_id, const std::string& exclustions = std::string()) {
                try {
                    if (exclustions.empty()) {
                        *m_dbhandle
                            << "SELECT id, typ, name, siz, hash FROM arc_tree WHERE parent_id=? ORDER BY typ DESC"
                            << parent_id
                            >> callback;
                    } else {
                        *m_dbhandle
                            << Glib::ustring::compose("SELECT id, typ, name, siz, hash, arc_media_id IN (%1) as A FROM arc_tree INNER JOIN arc_tree_to_media on id=arc_tree_id WHERE parent_id=? AND A=1 ORDER BY typ DESC", exclustions).operator std::string()
                            << parent_id
                            >> callback;
                    }
                } catch (const std::exception& e) {
                    assert_fail(e);
                }
            }

            template<typename F>
            void browseMedia(F callback) {
                try {
                    *m_dbhandle
                        << "SELECT id, capacity, occupied, name, serial FROM arc_media"
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

            class Media {
                public:
                    Media(std::unique_ptr<sqlite::database>& dbhandle, uint64_t id);

                    std::unique_ptr<Media> getMedia(uint64_t id);

                    void occupy(uint64_t size);
                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] const Glib::ustring& serial() const;
                    [[nodiscard]] uint64_t occupied() const;
                    [[nodiscard]] uint64_t capacity() const;
                    [[nodiscard]] uint64_t free() const;
                    [[nodiscard]] uint64_t id() const;
                    void remove();

                private:
                    Glib::ustring m_name;
                    Glib::ustring m_serial;
                    uint64_t m_capacity {0};
                    uint64_t m_occupied {0};
                    uint64_t m_id{0};
                    bool m_locked {false};
                    std::unique_ptr<sqlite::database>& m_dbhandle;

                friend class Archive;
            };

        private:

            class Settings {
                public:
                    explicit Settings(std::unique_ptr<sqlite::database>& dbhandle);
                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] const std::unique_ptr<Media>& media() const;
                    void updateName(const Glib::ustring& name);
                    void switchMedia(uint64_t new_id);

                private:
                    Glib::ustring m_name;
                    std::unique_ptr<sqlite::database>& m_dbhandle;
                    std::unique_ptr<Media> m_current_media;

                    friend class Archive;
            };

            std::string m_dbname;
            std::unique_ptr<sqlite::database> m_dbhandle;

    public:
            std::unique_ptr<Settings> settings;

    };


} // arc

#endif //COLD_ARC_GTK_ARCHIVE_H
