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

            static cold_arc::Result<> configure();

            static Archive& instance();

            using ArchivePtr = std::unique_ptr<Archive>;

            [[nodiscard]] cold_arc::Result<> beginTransaction();
            [[nodiscard]] cold_arc::Result<> commitTransaction();
            [[nodiscard]] cold_arc::Result<> rollbackTransaction();

            /**
             * Clones archive instance for local use
             *
             * @return clone ptr
             */
            [[nodiscard]] static cold_arc::Result<ArchivePtr> clone();
            /**
             * Creates new archive
             *
             * @param filename arch file name
             */
            [[nodiscard]] cold_arc::Result<> newArchive(const std::string& filename);
            /**
             * Opens archive
             * @param filename archive file name
             */
            [[nodiscard]] cold_arc::Result<> openArchive(const std::string& filename);
            /**
             * Creates new media
             * @param name
             * @param serial
             * @param capacity
             */
            [[nodiscard]] cold_arc::Result<> newMedia(const std::string& name, const std::string& serial, uint64_t capacity, bool rockridge, bool joliet);
            /**
             * Initialize database handle and checks the database version
             *
             * @param filename database filename
             */
            [[nodiscard]] cold_arc::Result<> init(const std::string& filename);
            /**
             * Creates folder
             * @param name
             * @param parentId
             * @param quiet
             * @return folder id
             */
            [[nodiscard]] cold_arc::Result<uint64_t> createFolder(const std::string& name, uint64_t parentId = 1, bool quiet = false);
            /**
             * Creates path
             * @param path
             * @param parentId
             * @param quiet
             * @return the last creted folder id
             */
            [[nodiscard]] cold_arc::Result<uint64_t> createPath(const std::string& path, uint64_t parentId = 1, bool quiet = false);

            /**
             * Add information about certain file
             * @param name
             * @param file_info
             * @param parentId
             * @return
             */
            [[nodiscard]] cold_arc::Result<bool> createFile(const std::string& name, const UploadFileInfo& file_info, uint64_t parentId = 1);

            /**
             * Removes file or folder
             * @param id
             * @param media_ids
             */
            [[nodiscard]] cold_arc::Result<> remove(uint64_t id, const std::vector<uint64_t>& media_ids);

            enum WalkFlags {
                    CallbackLast =        (1 << 0),
                    WalkFilesAndFolders = (1 << 1),
            };

            template<typename F>
            [[nodiscard]] cold_arc::Result<> walkTree(F && callback, uint64_t parent_id, const std::string& exclusions = std::string(), int flags = 0) {
                cold_arc::Error e;
                walkTree(std::forward<F>(callback), parent_id, exclusions, flags, e);
                if (e.code != cold_arc::ErrorCode::None)
                    return unexpected_nested(cold_arc::ErrorCode::WalkTreeError, e);
                return {};
            }

            /**
             * Browse the tree using exclustion
             * @tparam F
             * @param callback
             * @param parent_id
             * @param exclusions
             */
            template<typename F>
            [[nodiscard]] cold_arc::Result<> browse(F && callback, uint64_t parent_id, const std::string& exclusions = std::string()) {
                try {
                    cold_arc::Error er;
                    if (exclusions.empty()) {
                        *m_dbhandle
                            << "SELECT id, typ, name, siz, hash, lnk FROM arc_tree WHERE parent_id=? ORDER BY typ DESC"
                            << parent_id
                            >> [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, sqlite3_uint64 siz, const std::string& hash, const std::string& lnk) {
                                er = callback(id, typ, name, siz, hash, lnk);
                               };
                    } else {
                        *m_dbhandle
                            << Glib::ustring::compose("SELECT id, typ, name, siz, hash, lnk, arc_media_id IN (%1) as A FROM arc_tree INNER JOIN arc_tree_to_media on id=arc_tree_id WHERE parent_id=? AND A=1 GROUP BY id ORDER BY typ DESC", exclusions).operator std::string()
                            << parent_id
                            >> [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, sqlite3_uint64 siz, const std::string& hash, const std::string& lnk) {
                                er = callback(id, typ, name, siz, hash, lnk);
                               };
                    }
                    if (er.code != cold_arc::ErrorCode::None)
                        return unexpected_nested(cold_arc::ErrorCode::BrowseError, er);
                    return {};
                } catch (const sqlite::sqlite_exception& e) {
                    return unexpected_explained(cold_arc::ErrorCode::BrowseError, cold_arc::explain_sqlite_error, e.get_code());
                }
            }

            /**
             * Browse media
             * @tparam F
             * @param callback
             */
            template<typename F>
            [[nodiscard]] cold_arc::Result<> browseMedia(F && callback) {
                try {
                    cold_arc::Error er;
                    *m_dbhandle
                        << "SELECT id, capacity, occupied, name, serial, locked FROM arc_media"
                        >> [&](sqlite3_uint64 id, sqlite3_uint64 cap, sqlite3_uint64 occ, const std::string& name, const std::string& serial, sqlite3_uint64 locked) {
                            er = callback(id, cap, occ, name, serial, locked);
                           };
                    if (er.code != cold_arc::ErrorCode::None)
                        return unexpected_nested(cold_arc::ErrorCode::BrowseMediaError, er);
                    return {};
                } catch (const sqlite::sqlite_exception& e) {
                    return unexpected_explained(cold_arc::ErrorCode::BrowseMediaError, cold_arc::explain_sqlite_error, e.get_code());
                }
            }

            /**
             * Helper function to get media id from file or folder id
             * @tparam F
             * @param arc_id
             * @param callback
             */
            template<typename F>
            [[nodiscard]] cold_arc::Result<> getMediaForArcId(uint64_t arc_id, F && callback) {
                try {
                    cold_arc::Error er;
                    *m_dbhandle
                        << "SELECT arc_media_id FROM arc_tree_to_media WHERE arc_tree_id=?"
                        << arc_id
                        >> [&](sqlite3_uint64 id) {
                                er = callback(id);
                           };
                    if (er.code != cold_arc::ErrorCode::None)
                        return unexpected_nested(cold_arc::ErrorCode::GetMediaForArcIdError, er);
                    return {};
                } catch (const sqlite::sqlite_exception& e) {
                    return unexpected_explained(cold_arc::ErrorCode::GetMediaForArcIdError, cold_arc::explain_sqlite_error, e.get_code());
                }
            }

            template<typename F>
            [[nodiscard]] cold_arc::Result<> walkRoot(F && callback, uint64_t id) {
                cold_arc::Error e;
                walkRoot(std::forward<F>(callback), id, e);
                if (e.code != cold_arc::ErrorCode::None)
                    return unexpected_nested(cold_arc::ErrorCode::WalkRootError, e);
                return {};
            }

            [[nodiscard]] bool hasActiveArchive() const;
            [[nodiscard]] bool hasCurrentMedia() const;

            class Media {
                public:
                    Media() = default;
                    [[nodiscard]] cold_arc::Result<> construct(const std::shared_ptr<sqlite::database>& dbhandle, uint64_t id);
                    [[nodiscard]] cold_arc::Result<std::unique_ptr<Media>> construct(uint64_t id);

                    /**
                     * update occupied field
                     * @param size
                     */
                    [[nodiscard]] cold_arc::Result<> occupy(uint64_t size);
                    [[nodiscard]] cold_arc::Result<> lock(bool locked);
                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] const Glib::ustring& serial() const;
                    [[nodiscard]] uint64_t occupied() const;
                    [[nodiscard]] uint64_t capacity() const;
                    [[nodiscard]] uint64_t free() const;
                    [[nodiscard]] uint64_t id() const;
                    [[nodiscard]] bool rockridge() const;
                    [[nodiscard]] bool joliet() const;
                    [[nodiscard]] bool locked() const;
                    /**
                     * Removes the media
                     */
                    [[nodiscard]] cold_arc::Result<> remove();

                private:
                    Glib::ustring m_name;
                    Glib::ustring m_serial;
                    uint64_t m_capacity {0};
                    uint64_t m_occupied {0};
                    uint64_t m_id{0};
                    bool m_locked {false};
                    bool m_rockridge {false};
                    bool m_joliet {false};
                    std::shared_ptr<sqlite::database> m_dbhandle;

                friend class Archive;
            };

        private:

            /**
             * Walk to root from folder of file id
             * @tparam F
             * @param callback
             * @param id
             */
            template<typename F>
            void walkRoot(F && callback, uint64_t id, cold_arc::Error& e) {
                e = callback(id);
                if (e.code != cold_arc::ErrorCode::None)
                    return;
                try {
                    *m_dbhandle
                        << "SELECT parent_id FROM arc_tree WHERE id=?"
                        << id
                        >> [&](sqlite3_uint64 parent_id) {
                            if (parent_id != 0 && e.code == cold_arc::ErrorCode::None) {
                                walkRoot(callback, parent_id, e);
                            }
                        };
                } catch (const sqlite::sqlite_exception& ex) {
                    if (e.code == cold_arc::ErrorCode::None)
                        e = {cold_arc::ErrorCode::WalkRootError, cold_arc::explain_sqlite_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, ex.get_code()};
                    else {
                        e = {cold_arc::ErrorCode::WalkRootComplexError, cold_arc::explain_nested_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, e};
                    }
                }
            }

            /**
             * Walks the tree from specified parent id
             * @tparam F
             * @param callback
             * @param parent_id
             * @param exclusions
             * @param flags
             */
            template<typename F>
            void walkTree(F && callback, uint64_t parent_id, const std::string& exclusions, int flags, cold_arc::Error& e) {
                auto lcall = [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk, sqlite3_uint64 dt) {
                    if (flags & CallbackLast) {
                        if (typ == "folder" && e.code == cold_arc::ErrorCode::None) {
                            walkTree(callback, id, exclusions, flags, e);
                            if (e.code != cold_arc::ErrorCode::None)
                                return;
                        }
                        e = callback(id, typ, name, hash, lnk, dt, parent_id);
                    } else {
                        e = callback(id, typ, name, hash, lnk, dt, parent_id);
                        if (e.code != cold_arc::ErrorCode::None)
                            return;
                        if (typ == "folder")
                            walkTree(callback, id, exclusions, flags, e);
                    }
                };

                try {
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
                } catch (const sqlite::sqlite_exception& ex) {
                    if (e.code == cold_arc::ErrorCode::None)
                        e = {cold_arc::ErrorCode::WalkTreeError, cold_arc::explain_sqlite_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, ex.get_code()};
                    else {
                        e = {cold_arc::ErrorCode::WalkTreeComplexError, cold_arc::explain_nested_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, e};
                    }
                }
            }

            class Settings {
                public:
                    Settings() = default;
                    [[nodiscard]] cold_arc::Result<> construct(const std::shared_ptr<sqlite::database>& dbhandle);
                    [[nodiscard]] const Glib::ustring& name() const;
                    [[nodiscard]] const std::unique_ptr<Media>& media() const;
                    /**
                     * Updates name of arc
                     * @param name
                     */
                    [[nodiscard]] cold_arc::Result<> updateName(const std::string& name);
                    /**
                     * Switch media
                     * @param new_id
                     */
                    [[nodiscard]] cold_arc::Result<> switchMedia(uint64_t new_id);
                    [[nodiscard]] cold_arc::Result<> reloadMedia();

                private:
                    Glib::ustring m_name;
                    std::shared_ptr<sqlite::database> m_dbhandle;
                    std::unique_ptr<Media> m_current_media;

                    friend class Archive;
            };

            std::string m_dbname;
            std::shared_ptr<sqlite::database> m_dbhandle;
            std::mutex m_transaction_lock;

    public:
            std::unique_ptr<Settings> settings;

    };


} // arc

#endif //COLD_ARC_GTK_ARCHIVE_H
