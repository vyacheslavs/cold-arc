//
// Created by developer on 8/6/23.
//

#include <iostream>
#include <glibmm/refptr.h>
#include <glibmm/bytes.h>
#include "Archive.h"
#include "Utils.h"
#include <giomm/resource.h>
#include <fstream>
#include <gtkmm/messagedialog.h>
#include "Signals.h"
#include "Exceptions.h"
#include <ctime>
#include <filesystem>

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

            m_dbhandle = std::make_unique<SqLiteHandle>(fn);
            settings = std::make_unique<Settings>(m_dbhandle);

            Signals::instance().update_main_window.emit();
            Signals::instance().update_tree.emit();
        } catch (const WrongDatabaseVersion& e) {
            Gtk::MessageDialog dlg(Glib::ustring::compose("Failed to open archive %1", filename));
            dlg.set_secondary_text(e.what());
            dlg.run();
        }
    }

    void Archive::newMedia(const Glib::ustring &name, const Glib::ustring &serial, int capacity) {
        auto rowid = m_dbhandle->insertInto("arc_media")
            .set("name", name)
            .set("serial", serial)
            .set("capacity", capacity)
            .set("occupied", 0)
            .set("locked", 0)
            .done();
        settings->m_currentMedia = std::make_unique<Media>(name, serial, capacity);
        settings->updateCurrentMedia(rowid);
    }

    bool Archive::hasActiveArchive() const {
        return m_dbhandle.operator bool();
    }

    bool Archive::hasCurrentMedia() const {
        if (!settings)
            return false;
        return settings->m_currentMediaId > 0;
    }

    void Archive::createFolder(const Glib::ustring &name, uint64_t parentId) {
        try {
            auto idx = m_dbhandle->insertInto("arc_tree")
                    .set("parent_id", parentId)
                    .set("typ", "folder")
                    .set("name", name)
                    .set("dt", std::time(nullptr))
                    .done();

            walkRoot([&](sqlite3_uint64 id) {
                m_dbhandle->insertInto("arc_tree_to_media")
                        .set("arc_tree_id", id)
                        .set("arc_media_id", settings->m_currentMediaId)
                        .ignoreConstraintError()
                        .done();
            }, idx);

            Signals::instance().new_folder.emit(name, idx, parentId);
        } catch (const InsertConstraint&) {
            Gtk::MessageDialog dlg(Glib::ustring::compose("Failed to create a new folder: %1", name));
            dlg.set_secondary_text("Probably such folder already exists");
            dlg.run();
        }
    }

    uint64_t Archive::createPath(const Glib::ustring& path, uint64_t parentId) {
        if (path.empty())
            return parentId;

        std::filesystem::path fspath(path);
        for (const auto& particle : fspath) {
            if (particle != "/") {
                std::cout << "["<<particle<<"]\n";
                
            }
        }

        return parentId;
    }

    template <>
    sqlite3_int64 SqLiteHandle::sql_column_type(sqlite3_stmt* stmt, int column) {
        return sqlite3_column_int64(stmt, column);
    }

    template <>
    sqlite3_uint64 SqLiteHandle::sql_column_type(sqlite3_stmt* stmt, int column) {
        return sqlite3_column_int64(stmt, column);
    }

    template <>
    const char* SqLiteHandle::sql_column_type(sqlite3_stmt *stmt, int column) {
        return (const char*)sqlite3_column_text(stmt, column);
    }

    Archive::Settings::Settings(std::unique_ptr<SqLiteHandle> &_settings) : m_dbhandle(_settings) {
        m_dbhandle->select("SELECT name, current_media FROM db_settings", [&](const char* text, sqlite3_uint64 id) {
            m_name = text;
            m_currentMediaId = id;
        });

        if (m_currentMediaId > 0) {
            m_dbhandle->select(Glib::ustring::compose("SELECT capacity, occupied, locked, name, serial FROM arc_media WHERE id=%1", m_currentMediaId),
                               [&](sqlite3_uint64 capacity, sqlite3_uint64 occupied, sqlite3_uint64 locked, const char* name, const char* serial) {
                m_currentMedia = std::make_unique<Media>(name, serial, capacity, occupied, locked);
            });
        }
    }

    const Glib::ustring &Archive::Settings::name() const {
        return m_name;
    }

    void Archive::Settings::updateName(const Glib::ustring &name) {
        m_name = name;
        m_dbhandle->update("db_settings")
            .set("name", name)
            .done();

        Signals::instance().update_main_window.emit();
    }

    void Archive::Settings::updateCurrentMedia(sqlite3_uint64 id) {
        m_currentMediaId = id;
        m_dbhandle->update("db_settings")
            .set("current_media", id)
            .done();

        Signals::instance().update_main_window.emit();
    }

    const std::unique_ptr<Archive::Media> &Archive::Settings::media() const {
        return m_currentMedia;
    }

    const Glib::ustring &Archive::Media::name() const {
        return m_name;
    }

    const Glib::ustring &Archive::Media::serial() const {
        return m_serial;
    }
} // arc
