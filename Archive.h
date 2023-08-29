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
#include "Exceptions.h"

namespace arc {

    class Archive {
        private:
            Archive() = default;
        public:

            class SavePoint {
                public:
                    SavePoint() = delete;
                    SavePoint(const SavePoint&) = delete;
                    ~SavePoint();

                    void rollback();
                private:
                    explicit SavePoint(sqlite::database* db);
                    sqlite::database* m_dbhandle;
                    friend class Archive;
                    std::string m_savepoint_name;
                    bool m_auto_release {true};
            };

            SavePoint savePoint();

            static Archive& instance();
            /**
             * Clones archive instance for local use
             *
             * @return clone ptr
             * @throws sqlite::sqlite_exception, WrongDatabaseVersion
             */
            [[nodiscard]] static std::unique_ptr<Archive> clone();
            /**
             * Creates new archive
             *
             * @param filename arch file name
             * @throws sqlite::sqlite_exception, WrongDatabaseVersion, DBNotThreadSafe, ExtractResourceException
             */
            void newArchive(const Glib::ustring& filename);
            /**
             * Opens archive
             * @param filename archive file name
             * @throws sqlite::sqlite_exception, WrongDatabaseVersion, DBNotThreadSafe
             */
            void openArchive(const Glib::ustring& filename);
            /**
             * Creates new media
             * @param name
             * @param serial
             * @param capacity
             * @throws sqlite::sqlite_exception
             */
            void newMedia(const Glib::ustring& name, const Glib::ustring& serial, uint64_t capacity);
            /**
             * Initialize database handle and checks the database version
             *
             * @param filename database filename
             * @throws sqlite::sqlite_exception, WrongDatabaseVersion
             */
            void init(const Glib::ustring& filename);
            /**
             * Creates folder
             * @param name
             * @param parentId
             * @param quiet
             * @return folder id
             * @throws sqlite::sqlite_exception
             */
            uint64_t createFolder(const Glib::ustring& name, uint64_t parentId = 1, bool quiet = false);
            /**
             * Creates path
             * @param path
             * @param parentId
             * @param quiet
             * @return the last creted folder id
             * @throws sqlite::sqlite_exception
             */
            uint64_t createPath(const Glib::ustring& path, uint64_t parentId = 1, bool quiet = false);

            /**
             * Add information about certain file
             * @param name
             * @param file_info
             * @param parentId
             * @return
             * @throws sqlite::sqlite_exception
             */
            bool createFile(const Glib::ustring& name, const UploadFileInfo& file_info, uint64_t parentId = 1);

            /**
             * Removes file or folder
             * @param id
             * @param media_ids
             * @throws sqlite::sqlite_exception
             */
            void remove(uint64_t id, const std::vector<uint64_t>& media_ids);

            enum WalkFlags {
                    CallbackLast =        (1 << 0),
                    WalkFilesAndFolders = (1 << 1),
            };

            /**
             * Walks the tree from specified parent id
             * @tparam F
             * @param callback
             * @param parent_id
             * @param exclusions
             * @param flags
             * @throws sqlite::sqlite_exception
             */
            template<typename F>
            void walkTree(F callback, uint64_t parent_id, const std::string& exclusions = std::string(), int flags = 0) {
                auto lcall = [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk, sqlite3_uint64 dt) {
                    if (flags & CallbackLast) {
                        if (typ == "folder")
                            walkTree(callback, id, exclusions, flags);
                        callback(id, typ, name, hash, lnk, dt, parent_id);
                    } else {
                        callback(id, typ, name, hash, lnk, dt, parent_id);
                        if (typ == "folder")
                            walkTree(callback, id, exclusions, flags);
                    }
                };

                if (exclusions.empty()) {
                    *m_dbhandle
                        << Glib::ustring::compose("SELECT id, typ, name, hash, lnk, dt FROM arc_tree WHERE parent_id=?%1", !(flags & WalkFilesAndFolders) ? " and typ='folder'" : "").operator std::string()
                        << parent_id
                        >> lcall;
                        ;
                } else {
                    *m_dbhandle
                        << Glib::ustring::compose("SELECT id, typ, name, hash, lnk, dt, arc_media_id IN (%1) as A FROM arc_tree INNER JOIN arc_tree_to_media on id=arc_tree_id WHERE arc_tree.parent_id=? and A=1%2 GROUP BY id", exclusions, !(flags & WalkFilesAndFolders) ? " AND typ='folder'" : "").operator std::string()
                        << parent_id
                        >> lcall;
                }
            }

            /**
             * Browse the tree using exclustion
             * @tparam F
             * @param callback
             * @param parent_id
             * @param exclusions
             * @throws sqlite::sqlite_exception
             */
            template<typename F>
            void browse(F callback, uint64_t parent_id, const std::string& exclusions = std::string()) {
                if (exclusions.empty()) {
                    *m_dbhandle
                        << "SELECT id, typ, name, siz, hash FROM arc_tree WHERE parent_id=? ORDER BY typ DESC"
                        << parent_id
                        >> callback;
                } else {
                    *m_dbhandle
                        << Glib::ustring::compose("SELECT id, typ, name, siz, hash, arc_media_id IN (%1) as A FROM arc_tree INNER JOIN arc_tree_to_media on id=arc_tree_id WHERE parent_id=? AND A=1 GROUP BY id ORDER BY typ DESC", exclusions).operator std::string()
                        << parent_id
                        >> callback;
                }
            }

            /**
             * Browse media
             * @tparam F
             * @param callback
             * @throws sqlite::sqlite_exception
             */
            template<typename F>
            void browseMedia(F callback) {
                {
                    *m_dbhandle
                        << "SELECT id, capacity, occupied, name, serial FROM arc_media"
                        >> callback;
                }
            }

            /**
             * Helper function to get media id from file or folder id
             * @tparam F
             * @param arc_id
             * @param callback
             * @throws sqlite::sqlite_exception
             */
            template<typename F>
            void getMediaForArcId(uint64_t arc_id, F callback) {
                {
                    *m_dbhandle
                        << "SELECT arc_media_id FROM arc_tree_to_media WHERE arc_tree_id=?"
                        << arc_id
                        >> callback;
                }
            }

            /**
             * Walk to root from folder of file id
             * @tparam F
             * @param callback
             * @param id
             * @throws sqlite::sqlite_exception
             */
            template<typename F>
            void walkRoot(F callback, uint64_t id) {
                callback(id);
                {
                    *m_dbhandle
                        << "SELECT parent_id FROM arc_tree WHERE id=?"
                        << id
                        >> [&](sqlite3_uint64 parent_id) {
                            if (parent_id != 0)
                                walkRoot(callback, parent_id);
                        };
                }
            }

            [[nodiscard]] bool hasActiveArchive() const;
            [[nodiscard]] bool hasCurrentMedia() const;

            class Media {
                public:
                    Media(std::unique_ptr<sqlite::database>& dbhandle, uint64_t id);

                    std::unique_ptr<Media> getMedia(uint64_t id);

                    /**
                     * update occupied field
                     * @param size
                     * @throws sqlite::sqlite_exception
                     */
                    void occupy(uint64_t size);
                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] const Glib::ustring& serial() const;
                    [[nodiscard]] uint64_t occupied() const;
                    [[nodiscard]] uint64_t capacity() const;
                    [[nodiscard]] uint64_t free() const;
                    [[nodiscard]] uint64_t id() const;
                    /**
                     * Removes the media
                     */
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
                    /**
                     * Updates name of arc
                     * @param name
                     * @throws sqlite::sqlite_exception
                     */
                    void updateName(const Glib::ustring& name);
                    /**
                     * Switch media
                     * @param new_id
                     * @throws sqlite::sqlite_exception
                     */
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
