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

        Glib::RefPtr< const Glib::Bytes > blob = Gio::Resource::lookup_data_global("/main/skeleton.db");
        {
            std::ofstream of(fn);
            if (!of)
                throw std::runtime_error("failed to run database");
            gsize sz;
            const char* buf = reinterpret_cast<const char*>(blob->get_data(sz));
            of.write(buf, sz);
        }
        openArchive(fn);
    }

    void Archive::openArchive(const Glib::ustring &filename) {
        const auto fn = endsWith(filename, ".db") ? filename : filename+".db";

        m_dbhandle = std::make_unique<SqLiteHandle>(fn);
        settings = std::make_unique<Settings>(m_dbhandle);
    }

    void Archive::newMedia(const Glib::ustring &name, const Glib::ustring &serial, int capacity) {
        auto rowid = m_dbhandle->insertInto("arc_media")
            .set("name", name)
            .set("serial", serial)
            .set("capacity", capacity)
            .set("occupied", 0)
            .set("locked", 0)
            .done();

        settings->updateCurrentMedia(rowid);
    }

    template <>
    sqlite3_int64 SqLiteHandle::sql_column_type(sqlite3_stmt* stmt, int column) {
        return sqlite3_column_int64(stmt, column);
    }

    template <>
    const char* SqLiteHandle::sql_column_type(sqlite3_stmt *stmt, int column) {
        return (const char*)sqlite3_column_text(stmt, column);
    }

    Archive::Settings::Settings(std::unique_ptr<SqLiteHandle> &_settings) : m_dbhandle(_settings) {
        m_dbhandle->select("SELECT name FROM db_settings", [&](const char* text) {
            m_name = text;
        });
    }

    const Glib::ustring &Archive::Settings::name() const {
        return m_name;
    }

    void Archive::Settings::updateName(const Glib::ustring &name) {
        m_name = name;
        m_dbhandle->update("db_settings")
            .set("name", name)
            .done();
    }

    void Archive::Settings::updateCurrentMedia(sqlite3_uint64 id) {
        m_current_media_id = id;
        m_dbhandle->update("db_settings")
            .set("current_media", id)
            .done();
    }
} // arc
