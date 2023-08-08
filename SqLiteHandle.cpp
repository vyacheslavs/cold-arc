//
// Created by developer on 8/7/23.
//

#include <iostream>
#include "SqLiteHandle.h"
#include "Exceptions.h"

namespace arc {
    SqLiteHandle::SqLiteHandle(const Glib::ustring &filename) {
        auto rc = sqlite3_open(filename.c_str(), &m_db);
        if (rc)
            throw std::runtime_error(Glib::ustring::compose("failed to open database: %1: code %2", filename, rc));

        // verify database version
        select("SELECT version FROM db_settings", [](sqlite3_int64 v) {
            if (v != db_version)
                throw WrongDatabaseVersion(v, db_version);
        });
    }

    SqLiteHandle::~SqLiteHandle() {
        if (m_db)
            sqlite3_close(m_db);
    }

    void SqLiteHandle::UpdateProxy::done() {
        Glib::ustring sql;

        auto cond_no = 0;
        sql = Glib::ustring::compose("UPDATE %1 SET ", m_table);
        for (const auto& p : m_values) {
            if (cond_no++>0) sql+= ", ";
            sql += Glib::ustring::compose("%1 = ?", p.first);
        }
        sqlite3_stmt* stmt = nullptr;
        const char* unused = nullptr;
        auto r = sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &stmt, &unused);
        if (r != SQLITE_OK)
            throw std::runtime_error(Glib::ustring::compose("sqlite3_prepare failed: %1", r));

        auto sqlite3_stmt_cleanup = [](sqlite3_stmt* ptr) { if (ptr) sqlite3_finalize(ptr); };
        std::unique_ptr<sqlite3_stmt, decltype(sqlite3_stmt_cleanup)> _cleanup(stmt, sqlite3_stmt_cleanup);

        auto index = 1;
        for (const auto&p : m_values) {
            std::visit([&](auto&& arg){
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, sqlite3_int64> || std::is_same_v<T, sqlite3_uint64>) {
                    if (sqlite3_bind_int64(stmt, index++, arg)!= SQLITE_OK)
                        throw std::runtime_error("sqlite3_bind_int64 failed to bind");
                } else if constexpr (std::is_same_v<T, Glib::ustring>) {
                    int ret;
                    if ((ret = sqlite3_bind_text(stmt, index++, arg.c_str(), arg.length(), nullptr))!=SQLITE_OK)
                        throw std::runtime_error(Glib::ustring::compose("sqlite3_bind_text failed to bind: %1", ret));
                }
            }, p.second);
        }

        if (sqlite3_step(stmt) != SQLITE_DONE)
            throw std::runtime_error("sqlite3 step failed on update");

    }

    sqlite3_int64 SqLiteHandle::InsertProxy::done() {
        Glib::ustring sql, values;
        sql = Glib::ustring::compose("INSERT INTO %1 (", m_table);
        values = ") VALUES (";
        auto cond_no = 0;

        for (const auto& p : m_values) {
            if (cond_no++>0) { sql+= ", "; values += ", "; }
            sql += Glib::ustring::compose("%1", p.first);
            values += "?";
        }

        sql += values + ")";

        sqlite3_stmt* stmt = nullptr;
        const char* unused = nullptr;
        auto r = sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &stmt, &unused);
        if (r != SQLITE_OK)
            throw std::runtime_error(Glib::ustring::compose("sqlite3_prepare failed: %1 for sql: %2", r, sql));

        auto sqlite3_stmt_cleanup = [](sqlite3_stmt* ptr) { if (ptr) sqlite3_finalize(ptr); };
        std::unique_ptr<sqlite3_stmt, decltype(sqlite3_stmt_cleanup)> _cleanup(stmt, sqlite3_stmt_cleanup);

        auto index = 1;
        for (const auto&p : m_values) {
            std::visit([&](auto&& arg){
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, sqlite3_int64> || std::is_same_v<T, sqlite3_uint64>) {
                    if (sqlite3_bind_int64(stmt, index++, arg)!= SQLITE_OK)
                        throw std::runtime_error("sqlite3_bind_int64 failed to bind");
                } else if constexpr (std::is_same_v<T, Glib::ustring>) {
                    int ret;
                    if ((ret = sqlite3_bind_text(stmt, index++, arg.c_str(), arg.length(), nullptr))!=SQLITE_OK)
                        throw std::runtime_error(Glib::ustring::compose("sqlite3_bind_text failed to bind: %1", ret));
                }
            }, p.second);
        }

        int ret;
        if ((ret = sqlite3_step(stmt)) != SQLITE_DONE) {
            if (ret == SQLITE_CONSTRAINT) {
                if (!m_ignoreConstraintError)
                    throw InsertConstraint();
            } else
                throw std::runtime_error(Glib::ustring::compose("sqlite3 step failed on insert: %1", ret));
        }

        return sqlite3_last_insert_rowid(m_db);
    }

    SqLiteHandle::InsertProxy &SqLiteHandle::InsertProxy::ignoreConstraintError() {
        m_ignoreConstraintError = true;
        return *this;
    }

} // arc