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

            m_dbhandle = std::make_unique<sqlite::database>(fn);
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
        *m_dbhandle
            << "INSERT INTO arc_media (name, serial, capacity, occupied, locked) VALUES (?,?,?,0,0)"
            << name.operator std::string() << serial.operator std::string() << capacity;

        settings->m_currentMedia = std::make_unique<Media>(name, serial, capacity);
        settings->updateCurrentMedia(m_dbhandle->last_insert_rowid());
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
            *m_dbhandle
                << "INSERT INTO arc_tree (parent_id, typ, name, dt) VALUES (?,?,?,?)"
                << parentId << "folder" << name.operator std::string() << std::time(nullptr);
            auto idx = m_dbhandle->last_insert_rowid();

            walkRoot([&](sqlite3_uint64 id) {
                try {
                    *m_dbhandle
                        << "INSERT INTO arc_tree_to_media (arc_tree_to_media, arc_media_id) VALUES (?,?)"
                        << id << settings->m_currentMediaId;
                } catch (const sqlite::exceptions::constraint& e) {} // ignore constraint errors
            }, idx);

            Signals::instance().new_folder.emit(name, idx, parentId);
        } catch (const sqlite::exceptions::constraint&) {
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

    Archive::Settings::Settings(std::unique_ptr<sqlite::database> &_settings) : m_dbhandle(_settings) {
        *m_dbhandle
            << "SELECT name, current_media FROM db_settings"
            >> [&](const std::string& text, sqlite3_uint64 id) {
            m_name = text;
            m_currentMediaId = id;
        };
        if (m_currentMediaId > 0) {
            *m_dbhandle
                << "SELECT capacity, occupied, locked, name, serial FROM arc_media WHERE id=?"
                << m_currentMediaId
                >> [&](sqlite3_uint64 capacity, sqlite3_uint64 occupied, sqlite3_uint64 locked, const std::string& name, const std::string& serial) {
                    m_currentMedia = std::make_unique<Media>(name, serial, capacity, occupied, locked);
                };
        }
    }

    const Glib::ustring &Archive::Settings::name() const {
        return m_name;
    }

    void Archive::Settings::updateName(const Glib::ustring &name) {
        m_name = name;
        *m_dbhandle
            << "UPDATE db_settings SET name=?"
            << name.operator std::string();
        Signals::instance().update_main_window.emit();
    }

    void Archive::Settings::updateCurrentMedia(sqlite3_uint64 id) {
        m_currentMediaId = id;
        *m_dbhandle
            << "UPDATE db_settings SET current_media=?"
            << id;
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
