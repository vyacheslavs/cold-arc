//
// Created by developer on 8/6/23.
//

#include <iostream>
#include "Archive.h"
#include "Utils.h"
#include <gtkmm/messagedialog.h>
#include "Signals.h"
#include "Exceptions.h"
#include <ctime>
#include <filesystem>
#include <cstdint>

namespace arc {
    Archive &Archive::instance() {
        static Archive _ins;
        return _ins;
    }

    void Archive::newArchive(const Glib::ustring &filename) {
        const auto fn = endsWith(filename, ".db") ? filename : filename+".db";

        if (fileExists(fn)) {
            Gtk::MessageDialog rusure("Database is already exist. Do you want to replace it?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO,
                                      true);
            if (rusure.run() == Gtk::RESPONSE_NO)
                return;
        }

        extractFromResource("/main/skeleton.db", fn);
        openArchive(fn);
    }

    void Archive::openArchive(const Glib::ustring &filename) {
        try {
            const auto fn = endsWith(filename, ".db") ? filename : filename+".db";

            init(fn);

            if (!sqlite3_threadsafe())
                throw std::runtime_error("sqlite3 is not thread safe");

            sqlite3_config(SQLITE_CONFIG_SERIALIZED);

            Signals::instance().update_main_window.emit();
            Signals::instance().update_tree.emit();
            Signals::instance().update_media_view.emit();
        } catch (const WrongDatabaseVersion& e) {
            Gtk::MessageDialog dlg(Glib::ustring::compose("Failed to open archive %1", filename));
            dlg.set_secondary_text(e.what());
            dlg.run();
        } catch (const std::exception& e) {
            assert_fail(e);
        }
    }

    void Archive::newMedia(const Glib::ustring &name, const Glib::ustring &serial, int capacity) {
        {
            *m_dbhandle
                << "INSERT INTO arc_media (name, serial, capacity, occupied, locked) VALUES (?,?,?,0,0)"
                << name.operator std::string() << serial.operator std::string() << capacity;
        }
        uint64_t media_id = m_dbhandle->last_insert_rowid();
        {
            *m_dbhandle
                << "UPDATE db_settings SET current_media=?"
                << media_id;
        }
        settings->m_current_media = std::make_unique<Media>(m_dbhandle, media_id, name, serial, capacity);

        Signals::instance().update_main_window.emit();
        Signals::instance().update_media_view.emit();
    }

    bool Archive::hasActiveArchive() const {
        return m_dbhandle.operator bool();
    }

    bool Archive::hasCurrentMedia() const {
        if (!settings)
            return false;
        return settings->m_current_media.operator bool();
    }

    uint64_t Archive::createFolder(const Glib::ustring& name, uint64_t parentId, bool quiet) {
        try {
            {
                *m_dbhandle
                    << "INSERT INTO arc_tree (parent_id, typ, name, dt) VALUES (?,?,?,?)"
                    << parentId << "folder" << name.operator std::string() << std::time(nullptr);
            }

            auto idx = m_dbhandle->last_insert_rowid();
            walkRoot([&](sqlite3_uint64 id) {
                try {
                    *m_dbhandle
                        << "INSERT INTO arc_tree_to_media (arc_tree_id, arc_media_id) VALUES (?,?)"
                        << id << settings->media()->id();
                } catch (const sqlite::exceptions::constraint& e) {}
            }, idx);

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
                    << name.operator std::string() << parentId
                    >> folder_id;
            } catch (const std::runtime_error& e) {
                assert_fail(e);
            }
            return folder_id;
        } catch (const std::exception& e) {
            assert_fail(e);
        }
    }

    uint64_t Archive::createPath(const Glib::ustring& path, uint64_t parentId, bool quiet) {
        if (path.empty())
            return parentId;

        std::filesystem::path fspath(path);
        for (const auto& particle : fspath) {
            if (particle != "/")
                parentId = createFolder(particle.c_str(), parentId, quiet);
        }
        return parentId;
    }

    bool Archive::createFile(const Glib::ustring& name, const UploadFileInfo& file_info, uint64_t parentId) {
        sqlite3_uint64 idx;
        bool file_created = true;
        try {
            *m_dbhandle
                << "INSERT INTO arc_tree (parent_id, typ, name, siz, hash, lnk, dt, dt_org, perm) VALUES (?, 'file', ?, ?, ?, ?, ?, ?, ?)"
                << parentId << name.operator std::string() << file_info.getSize() << file_info.getHash() << file_info.getPath()
                << std::time(nullptr) << file_info.getMtime() << file_info.getMode();
        } catch (const sqlite::exceptions::constraint&) { // ignore constraint errors
            file_created = false;
        } catch (const std::exception& e) {
            assert_fail(e);
        }
        idx = m_dbhandle->last_insert_rowid();
        walkRoot([&](sqlite3_uint64 id) {
            try {
                *m_dbhandle
                    << "INSERT INTO arc_tree_to_media (arc_tree_id, arc_media_id) VALUES (?,?)"
                    << id << settings->media()->id();
            } catch (const sqlite::exceptions::constraint& e) {}
        }, idx);
        return file_created;
    }

    std::unique_ptr<Archive> Archive::clone() {
        std::unique_ptr<Archive> newArchive (new Archive());
        newArchive->init(Archive::instance().m_dbname);
        return std::move(newArchive);
    }

    void Archive::init(const Glib::ustring& filename) {
        m_dbhandle = std::make_unique<sqlite::database>(filename, sqlite::sqlite_config{sqlite::OpenFlags::READWRITE | sqlite::OpenFlags::FULLMUTEX});
        settings = std::make_unique<Settings>(m_dbhandle);
        m_dbname = filename;

        sqlite3_uint64 v;
        *m_dbhandle
            << "SELECT version FROM db_settings"
            >> v;

        if (v != COLD_ARC_DB_VERSION)
            throw WrongDatabaseVersion(COLD_ARC_DB_VERSION, v);
    }

    Archive::Settings::Settings(std::unique_ptr<sqlite::database> &dbhandle) : m_dbhandle(dbhandle) {
        uint64_t media_id {0};
        *m_dbhandle
            << "SELECT name, current_media FROM db_settings"
            >> [&](const std::string& text, sqlite3_uint64 id) {
            m_name = text;
                media_id = id;
        };
        if (media_id > 0) {
            *m_dbhandle
                << "SELECT capacity, occupied, locked, name, serial FROM arc_media WHERE id=?"
                << media_id
                >> [&](sqlite3_uint64 capacity, sqlite3_uint64 occupied, sqlite3_uint64 locked, const std::string& name, const std::string& serial) {
                    m_current_media = std::make_unique<Media>(m_dbhandle, media_id, name, serial, capacity, occupied, locked);
                };
        }
    }

    const Glib::ustring &Archive::Settings::name() const {
        return m_name;
    }

    void Archive::Settings::updateName(const Glib::ustring &name) {
        m_name = name;
        {
            *m_dbhandle
                << "UPDATE db_settings SET name=?"
                << name.operator std::string();
        }
        Signals::instance().update_main_window.emit();
    }

    const std::unique_ptr<Archive::Media> &Archive::Settings::media() const {
        return m_current_media;
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

    void Archive::Media::occupy(uint64_t size) {
        assert(m_occupied + size <= m_capacity);
        m_occupied += size;
        try {
            *m_dbhandle
                << "UPDATE arc_media SET occupied=? WHERE id=?"
                << m_occupied << id();
        } catch (const std::exception& e) {
            assert_fail(e);
        }
    }

} // arc
