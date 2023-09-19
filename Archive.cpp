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
#include <zlib.h>
#include <regex>
#include "HistoryDialog.h"

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
        ss << cold_arc::explain_generic(e) << "wrong database version, loaded: "<<std::any_cast<sqlite3_uint64>(e.aux)<<", expected: "<<COLD_ARC_DB_VERSION<<"\n";
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
            auto ref = 0;
            {
                *m_dbhandle
                    << "SELECT siz, arc_media_id FROM arc_tree INNER JOIN arc_tree_to_media ON (arc_tree.id=arc_tree_to_media.arc_tree_id) WHERE arc_tree.id=?"
                    << id
                    >> [&](sqlite3_uint64 siz, sqlite3_uint64 mid) {
                        ref++;
                        if (media_set.count(mid)) {
                            {
                                *m_dbhandle << "UPDATE arc_media SET occupied=occupied-? WHERE id=?" << siz << mid;
                            }
                            {
                                *m_dbhandle
                                    << "DELETE FROM arc_tree_to_media WHERE arc_tree_id=? AND arc_media_id=?"
                                    << id << mid;
                            }
                            ref--;
                        }
                    };
            }
            if (ref == 0) {
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
                << "SELECT name, current_media, paranoic FROM db_settings"
                >> [&](const std::string& text, sqlite3_uint64 id, sqlite3_uint64 p) {
                    m_name = text;
                    m_paranoic = p;
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

    cold_arc::Result<> Archive::Settings::update(const std::string& name, bool paranoic) {
        try {
            *m_dbhandle
                << "UPDATE db_settings SET name=?, paranoic=?"
                << name
                << (paranoic ? 1 : 0);
            m_name = name;
            m_paranoic = paranoic;
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
    cold_arc::Result<> Archive::Settings::reloadMedia() {
        if (!m_current_media)
            return {};

        auto m = std::make_unique<Media>();
        if (auto res = m->construct(m_dbhandle, m_current_media->m_id); !res)
            return unexpected_nested(cold_arc::ErrorCode::ReloadMediaError, res.error());
        m_current_media = std::move(m);
        return {};
    }
    bool Archive::Settings::is_paranoic() const {
        return m_paranoic;
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
                << (m_occupied+size) << id();
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
    cold_arc::Result<> Archive::Media::lock(bool locked) {
        try {
            {
                *m_dbhandle
                    << "UPDATE arc_media SET locked=? WHERE id=?"
                    << (locked ? 1 : 0)
                    << m_id;
            }
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_sqlite_exception(cold_arc::ErrorCode::MediaLockError, e.get_code());
        }
        return {};
    }
    bool Archive::Media::locked() const {
        return m_locked;
    }

    static std::string explain_commite_pipe_error(const cold_arc::Error& e) {
        std::stringstream ss;
        ss << cold_arc::explain_generic(e);
        ss << "failed to invoke sqlite3 tool, propbably sqlite3 is not installed\n";
        return ss.str();
    }

    static std::string explain_libz_error(const cold_arc::Error& e) {
        std::stringstream ss;
        ss << cold_arc::explain_generic(e);
        ss << "failed to compress sql, error: " << std::any_cast<int>(e.aux) << "\n";
        return ss.str();
    }

    static void cleanup_sql(std::string& sql) {
        {
            std::regex history(R"((CREATE\s+TABLE\s+arc_history\s*\())");
            auto words_begin =
                std::sregex_iterator(sql.begin(), sql.end(), history);
            auto words_end = std::sregex_iterator();
            if (words_begin != words_end) {
                const std::smatch& match = *words_begin;
                auto remove_it = sql.begin() + match.position();
                while (remove_it != sql.end() && *remove_it != '\n') remove_it++;
                sql.erase(sql.begin() + match.position(), remove_it+1);
            }
        }

        do {
            std::regex history(R"((INSERT\s+INTO\s+arc_history))", std::regex_constants::multiline);
            auto words_begin = std::sregex_iterator(sql.begin(), sql.end(), history);
            auto words_end = std::sregex_iterator();
            if (words_begin == words_end)
                break;
            const std::smatch& match = *words_begin;
            auto remove_it = sql.begin() + match.position();
            while (remove_it != sql.end() && *remove_it != '\n') remove_it++;
            sql.erase(sql.begin() + match.position(), remove_it+1);

        } while (true);

        do {
            std::regex history(R"((INSERT\s+INTO\s+sqlite_sequence\s*VALUES\('arc_history'))", std::regex_constants::multiline);
            auto words_begin = std::sregex_iterator(sql.begin(), sql.end(), history);
            auto words_end = std::sregex_iterator();
            if (words_begin == words_end)
                break;
            const std::smatch& match = *words_begin;
            auto remove_it = sql.begin() + match.position();
            while (remove_it != sql.end() && *remove_it != '\n') remove_it++;
            sql.erase(sql.begin() + match.position(), remove_it+1);

        } while (true);

        auto transaction_start = sql.find("BEGIN TRANSACTION;");
        if (transaction_start != std::string::npos) {
            sql.insert(transaction_start + 18, "\n\n"
                "DROP TABLE db_settings;\n"
                "DROP INDEX arc_tree_walking;\n"
                "DROP TABLE arc_tree_to_media;\n"
                "DROP TABLE arc_tree;\n"
                "DROP TABLE arc_media;\n"
            );
        }

    }

    cold_arc::Result<uint64_t> Archive::commit(const std::string& desc) {
        // test if there's a sqlite tool invokable

        std::stringstream ss;
        ss << "echo .dump | sqlite3 "<<m_dbname;
        FILE* fp = popen(ss.str().c_str(), "r");
        if (!fp)
            return unexpected_explained(cold_arc::ErrorCode::CommitError, explain_commite_pipe_error, nullptr);
        auto pipe_auto_close = [](FILE* fp) {if (fp) fclose(fp); };
        std::unique_ptr<FILE, decltype(pipe_auto_close)> auto_pipe {fp, pipe_auto_close};

        if (auto res = beginTransaction(); !res)
            return unexpected_nested(cold_arc::ErrorCode::CommitError, res.error());
        cold_arc::Result<uint64_t> scope_error;
        do {
            ScopeExit scope([&]{
                if (!scope_error) {
                    if (auto rb = rollbackTransaction(); !rb)
                        scope_error = unexpected_combined_error(cold_arc::ErrorCode::CommitError, scope_error.error(), rb);
                    return;
                }
                if (auto res = commitTransaction(); !res)
                    scope_error = unexpected_nested(cold_arc::ErrorCode::CommitError, res.error());
            });

            const int bsize = 4096;
            std::list<std::vector<char>> bufs;
            size_t all_size = 0;
            do {
                std::vector<char> b;
                b.resize(bsize);
                auto n = fread(&b[0], 1, bsize, fp);
                if (n > 0) {
                    b.resize(n);
                    bufs.emplace_back(std::move(b));
                    all_size += n;
                }
                if (n != bsize)
                    break;
            } while (true);

            std::string sql;
            sql.reserve(all_size);
            for (auto& l : bufs) {
                sql += std::string(&l[0], l.size());
            }

            cleanup_sql(sql);

            std::vector<uint8_t> compressed(sql.size(), 0);
            uLongf compressed_sz = compressed.size();
            if (auto res = compress2(&compressed[0], &compressed_sz, reinterpret_cast<const Bytef*>(sql.data()), sql.size(), 9); res != Z_OK) {
                scope_error = unexpected_explained(cold_arc::ErrorCode::CommitError, explain_libz_error, res);
                break;
            }
            compressed.resize(compressed_sz);

            auto sha = sha256(sql);
            if (!sha) {
                scope_error = unexpected_nested(cold_arc::ErrorCode::CommitError, sha.error());
                break;
            }

            uint64_t history_id;
            bool last_record = false;
            try {
                *m_dbhandle << "select id, (select max(id) from arc_history)=id from arc_history where hash=?"
                << sha.value()
                >> [&](sqlite3_uint64 id, sqlite3_uint64 is_it) {
                    if (is_it) {
                        last_record = true;
                        history_id = id;
                    }
                };
            } catch (const sqlite::sqlite_exception& e) {
                scope_error = unexpected_sqlite_exception(cold_arc::ErrorCode::CommitError, e.get_code());
                break;
            }

            if (last_record) {
                // just update the description
                try {
                    *m_dbhandle << "UPDATE arc_history SET description=?, dt=?  WHERE id=?"
                    << desc << time(nullptr) << history_id;
                } catch (const sqlite::sqlite_exception& e) {
                    scope_error = unexpected_sqlite_exception(cold_arc::ErrorCode::CommitError, e.get_code());
                    break;
                }
            } else {
                try {
                    *m_dbhandle
                        << "INSERT INTO arc_history (description, dt, data, dsize, hash) VALUES (?, ?, ?, ?, ?)"
                        << desc
                        << time(nullptr)
                        << compressed
                        << sql.size()
                        << sha.value();
                } catch (const sqlite::sqlite_exception& e) {
                    scope_error = unexpected_sqlite_exception(cold_arc::ErrorCode::CommitError, e.get_code());
                    break;
                }
                history_id = m_dbhandle->last_insert_rowid();
            }

            set_cursor(history_id);
            scope_error = history_id;
        } while (false);


        return scope_error;
    }

    cold_arc::Result<> Archive::applyCommit(uint64_t id) {

        std::vector<uint8_t> data;
        sqlite3_uint64 dsize;
        try {
            *m_dbhandle
            << "SELECT data, dsize FROM arc_history WHERE id=?"
            << id
            >> [&](std::vector<uint8_t>&& v, sqlite3_uint64 sz) {
                data = std::move(v);
                dsize = sz;
            };
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_sqlite_exception(cold_arc::ErrorCode::ApplyCommitError, e.get_code());
        }
        if (data.empty())
            return unexpected_error(cold_arc::ErrorCode::ApplyCommitError);

        std::string sql;
        sql.resize(dsize);
        uLongf destLen = dsize;
        uLong srcLen = data.size();
        if (auto r = uncompress2(reinterpret_cast<Bytef*>(&sql[0]), &destLen, &data[0], &srcLen); r!= Z_OK)
            return unexpected_explained(cold_arc::ErrorCode::ApplyCommitError, explain_libz_error, r);

        const auto max_path = 256;
        char tmpf[max_path] = "/tmp/coldarc.XXXXXX";
        {
            auto tmp = mkstemp(tmpf);
            if (tmp < 0)
                return unexpected_error(cold_arc::ErrorCode::ApplyCommitError);
            if (write(tmp, sql.c_str(), sql.size()) != sql.size()) {
                close(tmp);
                return unexpected_error(cold_arc::ErrorCode::ApplyCommitError);
            }
            close(tmp);
        }

        std::stringstream ss;
        ss << "cat "<<tmpf<<" | sqlite3 "<<m_dbname;
        if (auto r = system(ss.str().c_str()); r != 0)
            return unexpected_error(cold_arc::ErrorCode::ApplyCommitError);
        return {};
    }

    cold_arc::Result<> Archive::removeCommit(uint64_t id) {
        try {
            *m_dbhandle << "DELETE FROM arc_history WHERE id=?" << id;
        } catch (const sqlite::sqlite_exception& e) {
            return unexpected_sqlite_exception(cold_arc::ErrorCode::RemoveCommitError, e.get_code());
        }
        return {};
    }

} // arc
