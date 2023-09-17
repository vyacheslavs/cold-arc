//
// Created by developer on 8/6/23.
//

#include <iostream>
#include "Archive.h"
#include "Utils.h"
#include <gtkmm/messagedialog.h>
#include "Signals.h"
#include <ctime>
#include <filesystem>
#include <chrono>

namespace arc {
    Archive &Archive::instance() {
        static Archive _ins;
        return _ins;
    }

    cold_arc::Result<> Archive::newArchive(const std::string &filename) {
        if (filename.empty())
            return unexpected_invalid_input_parameter(cold_arc::ErrorCode::NewArchiveError, "filename");

        const auto fn = endsWith(filename, ".db") ? filename : filename+".db";

        if (fileExists(fn)) {
            Gtk::MessageDialog rusure("Database is already exist. Do you want to replace it?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO,
                                      true);
            if (rusure.run() == Gtk::RESPONSE_NO)
                return {};
        }

        if (auto res = extractFromResource("/main/skeleton.db", fn); !res)
            return unexpected_nested(cold_arc::ErrorCode::NewArchiveExtractError, res.error());
        if (auto res = openArchive(fn); !res)
            return unexpected_nested(cold_arc::ErrorCode::NewArchiveError, res.error());
        return {};
    }

    cold_arc::Result<> Archive::openArchive(const std::string &filename) {
        if (filename.empty())
            return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ArchiveOpenError, "filename");

        const auto fn = endsWith(filename, ".db") ? filename : filename+".db";

        if (auto res = init(fn); !res)
            return unexpected_nested(cold_arc::ErrorCode::ArchiveOpenError, res.error());

        if (!sqlite3_threadsafe())
            return unexpected_error(cold_arc::ErrorCode::ArchiveThreadSafeModeError);

        Signals::instance().update_main_window.emit();
        Signals::instance().update_tree.emit();
        Signals::instance().update_media_view.emit();
        return {};
    }

    cold_arc::Result<> Archive::newMedia(const std::string& name, const std::string& serial, uint64_t capacity, bool rockridge, bool joliet) {
        try {
            if (name.empty())
                return unexpected_invalid_input_parameter(cold_arc::ErrorCode::NewMediaError, "name");
            if (serial.empty())
                return unexpected_invalid_input_parameter(cold_arc::ErrorCode::NewMediaError, "serial");
            if (capacity < 1'000'000)
                return unexpected_invalid_input_parameter(cold_arc::ErrorCode::NewMediaError, "capacity");
            {
                *m_dbhandle
                    << "INSERT INTO arc_media (name, serial, capacity, occupied, locked, rockridge, joliet) VALUES (?,?,?,0,0,?,?)"
                    << name << serial << capacity << (rockridge ? 1 : 0) << (joliet ? 1 : 0);
            }
            uint64_t media_id = m_dbhandle->last_insert_rowid();
            {
                *m_dbhandle
                    << "UPDATE db_settings SET current_media=?"
                    << media_id;
            }

            auto m = std::make_unique<Media>();
            if (auto res = m->construct(m_dbhandle, media_id); !res)
                return unexpected_nested(cold_arc::ErrorCode::NewMediaError, res.error());
            settings->m_current_media = std::move(m);
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::NewMediaError, cold_arc::explain_sqlite_error, e.get_code());
        } catch (const std::bad_alloc& e) {
            return unexpected_error(cold_arc::ErrorCode::NewMediaError);
        }

        Signals::instance().update_main_window.emit();
        Signals::instance().update_media_view.emit();
        return {};
    }

    bool Archive::hasActiveArchive() const {
        return m_dbhandle.operator bool();
    }

    bool Archive::hasCurrentMedia() const {
        if (!settings)
            return false;
        return settings->m_current_media.operator bool();
    }

    cold_arc::Result<uint64_t> Archive::createFolder(const std::string& name, uint64_t parentId, bool quiet) {
        try {
            if (name.empty())
                return unexpected_invalid_input_parameter(cold_arc::ErrorCode::CreateFolderError, "name");
            {
                *m_dbhandle
                    << "INSERT INTO arc_tree (parent_id, typ, name, dt) VALUES (?,?,?,?)"
                    << parentId << "folder" << name << std::time(nullptr);
            }

            auto idx = m_dbhandle->last_insert_rowid();
            if (auto walk = walkRoot([&](sqlite3_uint64 id) {
                try {
                    *m_dbhandle
                        << "INSERT INTO arc_tree_to_media (arc_tree_id, arc_media_id) VALUES (?,?)"
                        << id << settings->media()->id();
                } catch (const sqlite::exceptions::constraint& e) {
                } catch (const sqlite::sqlite_exception& e) {
                    return cold_arc::Error{cold_arc::ErrorCode::CreateFolderError, cold_arc::explain_sqlite_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, e.get_code()};
                }
                return cold_arc::Error{};
            }, idx); !walk)
                return unexpected_nested(cold_arc::ErrorCode::CreateFolderError, walk.error());

            if (!quiet)
                Signals::instance().new_folder.emit(name, idx, parentId);
            return idx;
        } catch (const sqlite::exceptions::constraint&) {
            if (!quiet) {
                Gtk::MessageDialog dlg(Glib::ustring::compose("Failed to create a new folder: %1", name));
                dlg.set_secondary_text("Probably such folder already exists");
                dlg.run();
            }
            sqlite3_uint64 folder_id;
            try {
                *m_dbhandle
                    << "SELECT id FROM arc_tree WHERE name=? AND parent_id=?"
                    << name << parentId
                    >> folder_id;
            } catch (const sqlite::sqlite_exception& e) {
                return unexpected_explained(cold_arc::ErrorCode::CreateFolderError, cold_arc::explain_sqlite_error, e.get_code());
            }
            return folder_id;
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::CreateFolderError, cold_arc::explain_sqlite_error, e.get_code());
        }
    }

    cold_arc::Result<uint64_t> Archive::createPath(const std::string& path, uint64_t parentId, bool quiet) {
        if (path.empty())
            return parentId;

        std::filesystem::path fspath(path);
        for (const auto& particle : fspath) {
            if (particle != "/") {
                auto res = createFolder(particle.c_str(), parentId, quiet);
                if (!res)
                    return unexpected_nested(cold_arc::ErrorCode::CreatePathError, res.error());
                parentId = res.value();
            }
        }
        return parentId;
    }

    cold_arc::Result<bool> Archive::createFile(const std::string& name, const UploadFileInfo& file_info, uint64_t parentId) {
        sqlite3_uint64 idx;
        bool file_created = true;
        if (name.empty())
            return unexpected_invalid_input_parameter(cold_arc::ErrorCode::CreateFileError, "name");
        try {
            *m_dbhandle
                << "INSERT INTO arc_tree (parent_id, typ, name, siz, hash, lnk, dt, dt_org, perm) VALUES (?, 'file', ?, ?, ?, ?, ?, ?, ?)"
                << parentId << name << file_info.getSize() << file_info.getHash() << file_info.getPath()
                << std::time(nullptr) << file_info.getMtime() << file_info.getMode();
        } catch (const sqlite::exceptions::constraint&) { // ignore constraint errors
            file_created = false;
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::CreateFileError, cold_arc::explain_sqlite_error, e.get_code());
        }
        idx = m_dbhandle->last_insert_rowid();
        if (auto walk = walkRoot([&](sqlite3_uint64 id) {
            try {
                *m_dbhandle
                    << "INSERT INTO arc_tree_to_media (arc_tree_id, arc_media_id) VALUES (?,?)"
                    << id << settings->media()->id();
            } catch (const sqlite::exceptions::constraint& e) {
            } catch (const sqlite::sqlite_exception& e) {
                return cold_arc::Error{cold_arc::ErrorCode::CreateFileError, cold_arc::explain_sqlite_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, e.get_code()};
            }
            return cold_arc::Error{};
        }, idx); !walk)
            return unexpected_nested(cold_arc::ErrorCode::CreateFileError, walk.error());
        return file_created;
    }

    cold_arc::Result<Archive::ArchivePtr> Archive::clone() {
        try {
            std::unique_ptr<Archive> newArchive (new Archive());
            if (auto res = newArchive->init(Archive::instance().m_dbname); !res)
                return unexpected_nested(cold_arc::ErrorCode::ArchiveCloneError, res.error());
            return std::move(newArchive);
        } catch (const std::bad_alloc& e) {
            return unexpected_error(cold_arc::ErrorCode::ArchiveCloneError);
        }
    }

    static std::string explain_db_version_error(const cold_arc::Error& e) {
        std::stringstream ss;
        ss << cold_arc::explain_generic(e) << "wrong database version, loaded: "<<std::any_cast<sqlite3_int64>(e.aux)<<", expected: "<<COLD_ARC_DB_VERSION<<"\n";
        return ss.str();
    }

    cold_arc::Result<> Archive::init(const std::string& filename) {
        try {
            if (filename.empty())
                return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ArchiveInitError, "filename");

            auto handle = std::make_shared<sqlite::database>(filename, sqlite::sqlite_config{sqlite::OpenFlags::READWRITE | sqlite::OpenFlags::FULLMUTEX});

            sqlite3_uint64 v;
            {
                *handle
                    << "SELECT version FROM db_settings"
                    >> v;
            }

            if (v != COLD_ARC_DB_VERSION)
                return unexpected_explained(cold_arc::ErrorCode::ArchiveWrongDBVerionError, explain_db_version_error, v);

            auto s = std::make_unique<Settings>();
            if (auto res = s->construct(handle); !res)
                return unexpected_nested(cold_arc::ErrorCode::ArchiveInitError, res.error());

            m_dbhandle = handle;
            m_dbname = filename;
            settings = std::move(s);

            return {};
        } catch (const std::bad_alloc& e) {
            return unexpected_error(cold_arc::ErrorCode::ArchiveInitError);
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::ArchiveInitError, cold_arc::explain_sqlite_error, e.get_code());
        }
    }
    cold_arc::Result<> Archive::remove(uint64_t id, const std::vector<uint64_t>& media_ids) {
        try {
            std::set<uint64_t> media_set(media_ids.begin(), media_ids.end());
            {
                *m_dbhandle
                    << "SELECT siz, arc_media_id FROM arc_tree INNER JOIN arc_tree_to_media ON (arc_tree.id=arc_tree_to_media.arc_tree_id) WHERE arc_tree.id=?"
                    << id
                    >> [&](sqlite3_uint64 siz, sqlite3_uint64 mid) {
                        if (media_set.count(mid)) {
                            {
                                *m_dbhandle << "UPDATE arc_media SET occupied=occupied-? WHERE id=?" << siz << mid;
                            }
                            {
                                *m_dbhandle
                                    << "DELETE FROM arc_tree_to_media WHERE arc_tree_id=? AND arc_media_id=?"
                                    << id << mid;
                            }
                        }
                    };
            }
            {
                *m_dbhandle
                    << "DELETE FROM arc_tree WHERE id=?"
                    << id;
            }
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::RemoveError, cold_arc::explain_sqlite_error, e.get_code());
        } catch (const std::bad_alloc& e) {
            return unexpected_error(cold_arc::ErrorCode::RemoveError);
        }
        return {};
    }

    cold_arc::Result<> Archive::configure() {
        if (auto res = sqlite3_config(SQLITE_CONFIG_SERIALIZED); res < 0)
            return unexpected_explained(cold_arc::ErrorCode::ArchiveConfigureDB, cold_arc::explain_sqlite_error, res);
        return {};
    }
    cold_arc::Result<> Archive::beginTransaction() {
        m_transaction_lock.lock();
        try {
            *m_dbhandle << "BEGIN";
        } catch(const sqlite::sqlite_exception& e) {
            m_transaction_lock.unlock();
            return unexpected_explained(cold_arc::ErrorCode::BeginTransactionError, cold_arc::explain_sqlite_error, e.get_code());
        }
        return {};
    }
    cold_arc::Result<> Archive::commitTransaction() {
        try {
            *m_dbhandle << "COMMIT";
        } catch(const sqlite::sqlite_exception& e) {
            m_transaction_lock.unlock();
            return unexpected_explained(cold_arc::ErrorCode::CommitTransactionError, cold_arc::explain_sqlite_error, e.get_code());
        }
        m_transaction_lock.unlock();
        return {};
    }
    cold_arc::Result<> Archive::rollbackTransaction() {
        try {
            *m_dbhandle << "ROLLBACK";
        } catch(const sqlite::sqlite_exception& e) {
            m_transaction_lock.unlock();
            return unexpected_explained(cold_arc::ErrorCode::RollbackTransactionError, cold_arc::explain_sqlite_error, e.get_code());
        }
        m_transaction_lock.unlock();
        return {};
    }

    cold_arc::Result<> Archive::Settings::construct(const std::shared_ptr<sqlite::database>& dbhandle) {
        try {
            uint64_t media_id {0};
            if (!dbhandle)
                return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ConstructSettingsError, "dbhandle");
            *dbhandle
                << "SELECT name, current_media FROM db_settings"
                >> [&](const std::string& text, sqlite3_uint64 id) {
                    m_name = text;
                    media_id = id;
                };
            if (media_id > 0) {
                auto m = std::make_unique<Media>();
                if (auto res = m->construct(dbhandle, media_id); !res)
                    return unexpected_nested(cold_arc::ErrorCode::ConstructSettingsError, res.error());
                m_current_media = std::move(m);
            }
            m_dbhandle = dbhandle;
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::ConstructSettingsError, cold_arc::explain_sqlite_error, e.get_code());
        }
        return {};
    }

    const Glib::ustring &Archive::Settings::name() const {
        return m_name;
    }

    cold_arc::Result<> Archive::Settings::updateName(const std::string& name) {
        try {
            *m_dbhandle
                << "UPDATE db_settings SET name=?"
                << name;
            m_name = name;
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::SettingsUpdateNameError, cold_arc::explain_sqlite_error, e.get_code());
        }
        Signals::instance().update_main_window.emit();
        return {};
    }

    const std::unique_ptr<Archive::Media> &Archive::Settings::media() const {
        return m_current_media;
    }

    cold_arc::Result<> Archive::Settings::switchMedia(uint64_t new_id) {
        try {
            *m_dbhandle
                << "UPDATE db_settings SET current_media=?"
                << new_id;
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::SettingsSwitchMediaError, cold_arc::explain_sqlite_error, e.get_code());
        }
        auto m = std::make_unique<Media>();
        if (auto res = m->construct(m_dbhandle, new_id); !res)
            return unexpected_nested(cold_arc::ErrorCode::SettingsSwitchMediaError, res.error());
        m_current_media = std::move(m);
        Signals::instance().update_media_view.emit();
        return {};
    }

    const Glib::ustring &Archive::Media::name() const {
        return m_name;
    }

    const Glib::ustring &Archive::Media::serial() const {
        return m_serial;
    }

    uint64_t Archive::Media::id() const {
        return m_id;
    }

    uint64_t Archive::Media::occupied() const {
        return m_occupied;
    }

    uint64_t Archive::Media::capacity() const {
        return m_capacity;
    }

    uint64_t Archive::Media::free() const {
        assert(m_capacity >= m_occupied);
        return m_capacity - m_occupied;
    }

    cold_arc::Result<> Archive::Media::occupy(uint64_t size) {
        if (m_occupied + size > m_capacity)
            return unexpected_invalid_input_parameter(cold_arc::ErrorCode::MediaOccupyError, "size");
        try {
            *m_dbhandle
                << "UPDATE arc_media SET occupied=? WHERE id=?"
                << m_occupied << id();
            m_occupied += size;
            return {};
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::MediaOccupyError, cold_arc::explain_sqlite_error, e.get_code());
        }
    }

    cold_arc::Result<> Archive::Media::construct(const std::shared_ptr<sqlite::database>& dbhandle, uint64_t id) {
        try {
            bool has_media = false;
            *dbhandle
                << "SELECT capacity, occupied, locked, name, serial, rockridge, joliet FROM arc_media WHERE id=?"
                << id
                >> [&](sqlite3_uint64 capacity, sqlite3_uint64 occupied, sqlite3_uint64 locked, const std::string& name, const std::string& serial, sqlite3_uint64 rockridge, sqlite3_uint64 joliet) {
                    m_capacity = capacity;
                    m_occupied = occupied;
                    m_locked = locked;
                    m_name = name;
                    m_serial = serial;
                    m_id = id;
                    m_rockridge = rockridge > 0;
                    m_joliet = joliet > 0;
                    has_media = true;
                };

            if (!has_media)
                return unexpected_error(cold_arc::ErrorCode::MediaConstructError);
            m_dbhandle = dbhandle;
            return {};
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::MediaConstructError, cold_arc::explain_sqlite_error, e.get_code());
        }
    }

    cold_arc::Result<std::unique_ptr<Archive::Media>> Archive::Media::construct(uint64_t id) {
        auto m = std::make_unique<Archive::Media>();
        if (auto res = m->construct(m_dbhandle, id); !res)
            return unexpected_nested(cold_arc::ErrorCode::MediaConstructError, res.error());
        return m;
    }

    cold_arc::Result<> Archive::Media::remove() {
        try {
            {
                *m_dbhandle
                    << "SELECT id, COUNT(id) AS A FROM arc_tree INNER JOIN arc_tree_to_media ON (id=arc_tree_id) GROUP BY id HAVING A=1 and arc_media_id=?"
                    << m_id
                    >> [&](sqlite3_uint64 del_id) {
                        *m_dbhandle
                            << "DELETE FROM arc_tree WHERE id=?"
                            << del_id;
                    };
            }

            {
                *m_dbhandle
                    << "DELETE FROM arc_tree_to_media WHERE arc_media_id=?"
                    << m_id;
            }

            {
                *m_dbhandle
                    << "DELETE FROM arc_media WHERE id=?"
                    << m_id;
            }
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_explained(cold_arc::ErrorCode::RemoveMediaError, cold_arc::explain_sqlite_error, e.get_code());
        }

        Signals::instance().update_main_window.emit();
        Signals::instance().update_tree.emit();
        Signals::instance().update_media_view.emit();

        return {};
    }
    bool Archive::Media::rockridge() const {
        return m_rockridge;
    }
    bool Archive::Media::joliet() const {
        return m_joliet;
    }
} // arc
