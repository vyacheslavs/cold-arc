//
// Created by developer on 9/7/23.
//

#include "ExportDialog.h"
#include "Utils.h"
#include "Signals.h"
#include "Archive.h"
#include <functional>
#include <utility>
#include "ExportModelColumns.h"

cold_arc::Result<> ExportDialog::run(uint64_t media_id, const std::string& iso_file) {
    if (auto res = runDialog<ExportDialog>("/main/export.glade", "export_dlg", media_id, iso_file); !res)
        return unexpected_nested(cold_arc::ErrorCode::ExportDialogError, res.error());
    return {};
}
ExportDialog::ExportDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(win) {}

void ExportDialog::workThreadMain() {

    cold_arc::Error scope_error;
    ScopeExit s([&]() {
        if (scope_error.code == cold_arc::ErrorCode::None)
            notifyThreadStopped();
        notifyThreadStopped(scope_error);
    });
    arc::Archive::ArchivePtr m_db;
    auto dbres = arc::Archive::clone();
    if (!dbres) {
        scope_error = make_nested_error(cold_arc::ErrorCode::ExportDialogError, dbres.error());
        return;
    }
    m_db = std::move(dbres.value());

    std::unique_ptr<ISOBuilder> iso_builder = std::make_unique<ISOBuilder>();
    if (auto ret = iso_builder->construct(); !ret) {
        scope_error = make_nested_error(cold_arc::ErrorCode::ExportDialogError, ret.error());
        return;
    }

    auto media = arc::Archive::instance().settings->media()->construct(m_media_id);
    if (!media) {
        scope_error = make_nested_error(cold_arc::ErrorCode::ExportDialogError, media.error());
        return;
    }

    if (auto ret = iso_builder->prepareImage(media.value()->serial()); !ret) {
        scope_error = make_nested_error(cold_arc::ErrorCode::ExportDialogError, ret.error());
        return;
    }
    m_stream_callback.opaque = this;
    m_stream_callback.on_open = [](IsoStream* stream, void* dlg) {
        auto* edlg = reinterpret_cast<ExportDialog*>(dlg);
        auto cb = file_stream_get_callback(stream);
        int progress = cb->total > 0 ? (static_cast<int>(100 * cb->offset / cb->total)) : 0;
        edlg->notifyProgress(progress, file_stream_get_filepath(stream));
    };
    m_stream_callback.on_read = [](IsoStream* stream, void* dlg) {
        auto* edlg = reinterpret_cast<ExportDialog*>(dlg);
        edlg->notifyLastFileProgress(file_stream_get_progress(stream), file_stream_get_filepath(stream));
    };

    iso_builder->set_stream_callback(&m_stream_callback);

    std::map<uint64_t, IsoDir*> m_dirs;
    if (auto res = m_db->walkTree(
        [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk,
            sqlite3_uint64 dt, sqlite3_uint64 parent_id) {

            IsoDir* parent = iso_builder->root_folder();
            auto it = m_dirs.find(parent_id);
            if (it != m_dirs.end())
                parent = it->second;

            if (typ == "folder") {
                auto folder = iso_builder->new_folder(name, parent);
                if (!folder) {
                    return folder.error();
                }
                m_dirs[id] = folder.value();
            } else {
                if (auto nf = iso_builder->new_file(name, lnk, parent); !nf)
                    return nf.error();
            }
            return cold_arc::Error{};
        }, 1, std::to_string(m_media_id),arc::Archive::WalkFilesAndFolders); !res) {
        scope_error = make_nested_error(cold_arc::ErrorCode::ExportDialogError, res.error());
        return;
    }

    if (auto ret = iso_builder->burn(m_iso_filepath, media.value()->rockridge(), media.value()->joliet()); !ret) {
        scope_error = make_nested_error(cold_arc::ErrorCode::ExportDialogError, ret.error());
        return;
    }
}

void ExportDialog::onDispatcherNotification() {
    ExportProgressInfoQueue q;
    {
        auto cc = m_queue.access();
        q = std::move(*cc);
    }
    bool quit = false;
    for (const auto& item: q) {

        if (!item.running) {
            m_work_thread->join();
            m_work_thread.reset();
            quit = true;
        }

        if (item.e.code != cold_arc::ErrorCode::None) {
            quit = true;
            auto critical = true;
            if (cold_arc::get_nested<0>(item.e).code == cold_arc::ErrorCode::ExportDialogBurnError &&
                cold_arc::get_nested<1>(item.e).code == cold_arc::ErrorCode::ISOOutputFileCreateError) {
                critical = false;
            }
            if (critical)
                Signals::instance().app_quit.emit();

            reportError(item.e, critical);

        } else if (item.prog_type == progress_type::overall) {
            ExportModelColumns cols;

            if (!m_export_view->get_model()->children().empty()) {
                auto it = --m_export_view->get_model()->children().end();
                auto& row = *it;
                row[cols.percentage] = 100;
            }

            auto it = m_export_store->append();
            auto& row = *it;
            row[cols.percentage] = 0;
            row[cols.file_path] = item.file_name;
            m_progress_bar->set_fraction((double)item.progress / 100);
            m_export_view->scroll_to_row(m_export_view->get_model()->get_path(it));
        } else if (item.prog_type == progress_type::last_file) {
            ExportModelColumns cols;
            if (!m_export_view->get_model()->children().empty()) {
                auto it = --m_export_view->get_model()->children().end();
                auto& row = *it;
                row[cols.percentage] = item.progress;
            }
        }

        if (quit) {
            m_progress_bar->set_fraction(1);
            m_ok_btn->set_sensitive(true);
            break;
        }
    }
}
void ExportDialog::notifyThreadStopped() {
    {
        auto cc = m_queue.access();
        cc->emplace_back(false);
    }
    m_dispatcher.emit(DispatcherEmitPolicy::Force);
}
void ExportDialog::notifyProgress(int progress, std::string file) {
    {
        auto cc = m_queue.access();
        cc->emplace_back(progress, std::move(file));
    }
    m_dispatcher.emit(DispatcherEmitPolicy::Throttled);
}
void ExportDialog::notifyLastFileProgress(int progress, std::string file) {
    if (!m_dispatcher.timeToEmit())
        return;
    {
        auto cc = m_queue.access();
        cc->emplace_back(std::move(file), progress);
    }
    m_dispatcher.emit(DispatcherEmitPolicy::Force);
}
cold_arc::Result<> ExportDialog::construct(const Glib::RefPtr<Gtk::Builder>& builder, uint64_t media_id, std::string iso_file) {
    try {
        m_media_id = media_id;
        m_iso_filepath = std::move(iso_file);

        ExportModelColumns cols;
        m_export_view = findWidget<Gtk::TreeView>("export_view", builder);
        m_export_store = findObject<Gtk::ListStore>("liststore1", builder);
        m_progress_bar = findWidget<Gtk::ProgressBar>("progress_bar", builder);
        m_ok_btn = findWidget<Gtk::Button>("okbt", builder);

        auto progressRenderer = Gtk::manage(new Gtk::CellRendererProgress);
        Gtk::TreeView::Column* progressCol = Gtk::manage(new Gtk::TreeView::Column("Exported"));
        progressCol->pack_start(*progressRenderer, false);
        m_export_view->append_column(*progressCol);
        progressCol->add_attribute(progressRenderer->property_value(), cols.percentage);
        m_export_view->append_column("Path", cols.file_path);

        m_stream_callback.opaque = nullptr;
        m_stream_callback.offset = 0;
        m_stream_callback.total = 0;
        m_stream_callback.on_close = nullptr;
        m_stream_callback.on_read = nullptr;
        m_stream_callback.on_open = nullptr;
        m_dispatcher.connect(sigc::mem_fun(this, &ExportDialog::onDispatcherNotification));
        m_work_thread = std::make_unique<std::thread>(&ExportDialog::workThreadMain, this);
        m_ok_btn->set_sensitive(false);
        return {};
    } catch (const std::bad_alloc&) {
        return unexpected_error(cold_arc::ErrorCode::ExportDialogError);
    }

    return {};
}
