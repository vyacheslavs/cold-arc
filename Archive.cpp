//
// Created by developer on 8/6/23.
//

#include <iostream>
#include <glibmm/refptr.h>
#include <glibmm/bytes.h>
#include "Archive.h"
#include <giomm/resource.h>
#include <fstream>
#include <sys/stat.h>
#include <gtkmm/messagedialog.h>


namespace arc {
    Archive &Archive::instance() {
        static Archive _ins;
        return _ins;
    }

    void Archive::newArchive(const Glib::ustring &filename) {
        auto endswith = [](const Glib::ustring &filename, const Glib::ustring &postfix) {
            if (postfix.size() > filename.size()) return false;
            return std::equal(postfix.rbegin(), postfix.rend(), filename.rbegin());
        };
        const auto fn = endswith(filename, ".db") ? filename : filename+".db";

        // check if such db exists
        struct stat st {};
        if (stat(fn.c_str(), &st) == 0) {
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

        m_dbhandle = std::make_unique<SqLiteHandle>(filename);
        settings = std::make_unique<Settings>(m_dbhandle);
    }

    Archive::SqLiteHandle::SqLiteHandle(const Glib::ustring &filename) {
        auto rc = sqlite3_open(filename.c_str(), &m_db);
        if (rc)
            throw std::runtime_error(Glib::ustring::compose("failed to open database: %1: code %2", filename, rc));

        // verify database version
        select("SELECT version FROM db_settings", [](sqlite3_int64 v) {
            if (v != db_version)
                throw std::runtime_error("Wrong database version");
        });
    }

    Archive::SqLiteHandle::~SqLiteHandle() {
        if (m_db)
            sqlite3_close(m_db);
    }

    template <>
    sqlite3_int64 Archive::SqLiteHandle::sql_column_type(sqlite3_stmt* stmt, int column) {
        return sqlite3_column_int64(stmt, column);
    }

    template <>
    const char* Archive::SqLiteHandle::sql_column_type(sqlite3_stmt *stmt, int column) {
        return (const char*)sqlite3_column_text(stmt, column);
    }

    Archive::Settings::Settings(std::unique_ptr<SqLiteHandle> &_settings) : m_settings(_settings) {
        m_settings->select("SELECT name FROM db_settings", [&](const char* text) {
            m_name = text;
        });
    }

    const Glib::ustring &Archive::Settings::name() const {
        return m_name;
    }

    void Archive::Settings::updateSettings(const Glib::ustring &name) {
        m_name = name;
    }
} // arc
