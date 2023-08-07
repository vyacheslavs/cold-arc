//
// Created by developer on 8/6/23.
//

#ifndef COLD_ARC_GTK_ARCHIVE_H
#define COLD_ARC_GTK_ARCHIVE_H

#include <glibmm/ustring.h>
#include <memory>
#include "sqlite3.h"

namespace arc {

    class Archive {
        private:
            Archive() = default;
        public:
            static Archive& instance();
            void newArchive(const Glib::ustring& filename);

        private:
            class SqLiteHandle {
            public:
                explicit SqLiteHandle(const Glib::ustring& filename);
                ~SqLiteHandle();
            private:

                static auto constexpr db_version = 1;

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

                sqlite3* m_db {nullptr};
                friend class Archive;
            };

            class Settings {
                public:
                    explicit Settings(std::unique_ptr<SqLiteHandle>& _settings);
                    [[nodiscard]] const Glib::ustring& name() const;
                    void updateSettings(const Glib::ustring& name);
                private:
                    Glib::ustring m_name;
                    std::unique_ptr<SqLiteHandle>& m_settings;
            };

            std::unique_ptr<SqLiteHandle> m_dbhandle;

    public:
            std::unique_ptr<Settings> settings;

    };


} // arc

#endif //COLD_ARC_GTK_ARCHIVE_H
