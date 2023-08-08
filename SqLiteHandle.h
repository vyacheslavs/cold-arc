//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_SQLITEHANDLE_H
#define COLD_ARC_GTK_SQLITEHANDLE_H

#include <gtkmm-3.0/gtkmm.h>
#include <glibmm/ustring.h>
#include <sqlite3.h>
#include <utility>
#include <variant>

namespace arc {

    class SqLiteHandle {
    public:
        explicit SqLiteHandle(const Glib::ustring& filename);
        ~SqLiteHandle();
    private:

        static auto constexpr db_version = 8;

        template <class... T>
        struct pack { };

        template <class F>
        struct params : params<decltype(&F::operator())> { };

        template <class R, class T, class... Params>
        struct params<R (T::*)(Params...) const> {
            using type = pack<Params...>;
            using seq = std::index_sequence_for<Params...>;
        };

        template <typename C>
        C sql_column_type(sqlite3_stmt* stmt, int column);

        template<typename F>
        void select(const Glib::ustring& sql, F&& callback) {
            _select<F>(sql, std::forward<F>(callback), typename params<std::decay_t<F>>::type{}, typename params<std::decay_t<F>>::seq{});
        }

        template <typename F, typename... Params, std::size_t... Idx>
        void _select(const Glib::ustring& sql, F&& callback, pack<Params...>, std::index_sequence<Idx...> ind ) {
            sqlite3_stmt* stmt = nullptr;
            const char* unused = nullptr;
            auto r = sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &stmt, &unused);
            if (r != SQLITE_OK)
                throw std::runtime_error(Glib::ustring::compose("sqlite3_prepare failed: %1", r));

            auto sqlite3_stmt_cleanup = [](sqlite3_stmt* ptr) { if (ptr) sqlite3_finalize(ptr); };
            std::unique_ptr<sqlite3_stmt, decltype(sqlite3_stmt_cleanup)> _cleanup(stmt, sqlite3_stmt_cleanup);

            while ( (r = sqlite3_step(stmt)) == SQLITE_ROW) {
                callback(sql_column_type<Params>(stmt, Idx)...);
            }
        }

        using SqliteValue = std::variant<sqlite3_int64, sqlite3_uint64, Glib::ustring>;

        class UpdateProxy {
        public:

            UpdateProxy(Glib::ustring table, sqlite3* _db) : m_table(std::move(table)), m_db(_db) {}

            template<typename T>
            UpdateProxy& set(const Glib::ustring& column_name, T val) {
                m_values.emplace_back(column_name, val);
                return *this;
            }

            void done();
        private:
            Glib::ustring m_table;
            sqlite3* m_db {nullptr};
            std::vector<std::pair<Glib::ustring, SqliteValue>> m_values;
        };

        UpdateProxy update(const Glib::ustring& table) {
            return UpdateProxy(table, m_db);
        }

        class InsertProxy {
            public:
                InsertProxy(Glib::ustring  table, sqlite3* _db) : m_table(std::move(table)), m_db(_db) {};

                template<typename T>
                InsertProxy& set(const Glib::ustring& column_name, T val) {
                    m_values.emplace_back(column_name, val);
                    return *this;
                }

                InsertProxy& ignoreConstraintError();

                sqlite3_int64 done();

            private:
                Glib::ustring m_table;
                sqlite3* m_db {nullptr};
                bool m_ignoreConstraintError{false};

                std::vector<std::pair<Glib::ustring, SqliteValue>> m_values;
        };

        InsertProxy insertInto(const Glib::ustring& table) {
            return InsertProxy(table, m_db);
        }

        sqlite3* m_db {nullptr};
        friend class Archive;
    };
} // arc

#endif //COLD_ARC_GTK_SQLITEHANDLE_H
