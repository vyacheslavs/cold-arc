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

void ExportDialog::run(uint64_t media_id, const std::string& iso_file) {
    runDialog<ExportDialog>("/main/export.glade", "export_dlg", [](ExportDialog* dlg, int rc) {

    }, media_id, iso_file);
}
ExportDialog::ExportDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder, uint64_t media_id, std::string  iso_file) :
    Gtk::Dialog(win),
    m_media_id(media_id),
    m_iso_filepath(std::move(iso_file))
    {
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
}

void ExportDialog::workThreadMain() {

    std::unique_ptr<arc::Archive> m_db;
    try {
        m_db = arc::Archive::clone();
    } catch (const sqlite::sqlite_exception& e) {
        notifyThreadStopped(e);
        return;
    } catch (const WrongDatabaseVersion& e) {
        notifyThreadStopped(e);
        return;
    }

    try {
        std::unique_ptr<ISOBuilder> iso_builder = std::make_unique<ISOBuilder>();
        auto media = arc::Archive::instance().settings->media()->getMedia(m_media_id);
        iso_builder->prepareImage(media->serial().c_str());
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
        m_db->walkTree(
            [&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk,
                sqlite3_uint64 dt, sqlite3_uint64 parent_id) {

                IsoDir* parent = iso_builder->root_folder();
                auto it = m_dirs.find(parent_id);
                if (it != m_dirs.end())
                    parent = it->second;

                if (typ == "folder") {
                    m_dirs[id] = iso_builder->new_folder(name.c_str(), parent);
                } else {
                    (void) iso_builder->new_file(name.c_str(), lnk.c_str(), parent);
                }
            }, 1, std::to_string(m_media_id),arc::Archive::WalkFilesAndFolders);

        iso_builder->burn(m_iso_filepath.c_str(), media->rockridge(), media->joliet());

        notifyThreadStopped();
    } catch (const ISOBuilderException& e) {
        notifyThreadStopped(e);
    } catch (const sqlite::sqlite_exception& e) {
        notifyThreadStopped(e);
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

        if (item.e) {
            try {
                quit = true;
                item.e->rethrow();
            } catch (const ISOBuilderException& e) {
                Gtk::MessageDialog dlg("ISO builder error");
                dlg.set_secondary_text(e.what());
                dlg.run();
                Signals::instance().app_quit.emit();
            } catch (const sqlite::sqlite_exception& e) {
                sqliteError(e, true);
            }
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
