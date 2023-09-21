//
// Created by developer on 9/20/23.
//

#include "HistoryDialog.h"
#include "HistoryModelColumns.h"
#include "Archive.h"
#include "Signals.h"

static uint64_t cursor = 0;

HistoryDialog::HistoryDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(win) {}

cold_arc::Result<> HistoryDialog::run() {
    auto res = runDialog<HistoryDialog>("/main/history.glade", "history_dlg");
    if (!res)
        return unexpected_nested(cold_arc::ErrorCode::HistoryDialogError, res.error());

    return {};
}
cold_arc::Result<> HistoryDialog::construct(const Glib::RefPtr<Gtk::Builder>& builder) {
    m_treeview = findWidget<Gtk::TreeView>("history_list", builder);
    m_liststore = findObject<Gtk::ListStore>("liststore1", builder);
    m_apply_button = findWidget<Gtk::Button>("apply_button", builder);
    m_new_point_btn = findWidget<Gtk::Button>("new_save_point_button", builder);
    m_new_point_desc = findWidget<Gtk::Entry>("new_sv_desc", builder);
    m_remove_btn = findWidget<Gtk::Button>("remove_btn", builder);

    m_apply_button->signal_clicked().connect(sigc::mem_fun(this, &HistoryDialog::onApply));
    m_new_point_btn->signal_clicked().connect(sigc::mem_fun(this, &HistoryDialog::onNewSP));
    m_remove_btn->signal_clicked().connect(sigc::mem_fun(this, &HistoryDialog::onRemove));

    HistoryModelColumns cols;

    auto comboRenderer = Gtk::manage(new Gtk::CellRendererToggle);
    Gtk::TreeView::Column* pColumn = Gtk::manage(new Gtk::TreeView::Column(""));
    pColumn->pack_start(*comboRenderer, false);
    m_treeview->append_column(*pColumn);
    pColumn->add_attribute(comboRenderer->property_active(), cols.cursor);
    comboRenderer->set_activatable(false);

    m_treeview->append_column("Date/Time", cols.date_string);
    m_treeview->append_column("Description", cols.description);

    if (auto res = refresh(); !res)
        return unexpected_nested(cold_arc::ErrorCode::HistoryDialogError, res.error());

    return {};
}
cold_arc::Result<> HistoryDialog::refresh() {
    HistoryModelColumns cols;

    m_liststore->clear();

    if (auto res = arc::Archive::instance().walkHistory([&](sqlite3_uint64 id, const std::string& desc, sqlite3_uint64 dt) -> cold_arc::Error {
            auto n = m_liststore->append();
            n->set_value<guint64>(cols.id, id);
            n->set_value<guint64>(cols.date, dt);
            n->set_value<Glib::ustring>(cols.description, desc);
            n->set_value<gboolean>(cols.cursor, get_cursor().has_value() && get_cursor().value() == id);

            char date[100];
            struct tm *t = localtime(reinterpret_cast<const time_t*>(&dt));
            strftime(date, sizeof(date), "%Y-%m-%d %I:%M:%S %p", t);

            n->set_value<Glib::ustring>(cols.date_string, date);
            return {};
        }); !res)
        return unexpected_nested(cold_arc::ErrorCode::HistoryDialogError, res.error());

    return {};
}
void HistoryDialog::onApply() {
    if (!m_treeview->get_selection()->get_selected())
        return;

    HistoryModelColumns cols;
    auto n = m_treeview->get_selection()->get_selected();

    auto id = n->get_value<guint64>(cols.id);
    if (auto res = arc::Archive::instance().applyCommit(id); !res) {
        reportError(res.error());
        return;
    }

    cursor = id;

    Signals::instance().update_main_window.emit();
    Signals::instance().update_media_view.emit();
    Signals::instance().update_tree.emit();

    if (auto res = refresh(); !res)
        reportError(res.error());
}

void HistoryDialog::onNewSP() {
    auto text = m_new_point_desc->get_text();
    if (text.empty())
        text = "Save point created by user";
    auto res = arc::Archive::instance().commit(text);
    if (!res) {
        reportError(res.error());
        return;
    }
    if (auto r = refresh(); !r)
        reportError(r.error());
}

void HistoryDialog::onRemove() {
    if (!m_treeview->get_selection()->get_selected())
        return;

    HistoryModelColumns cols;
    auto n = m_treeview->get_selection()->get_selected();
    if (auto res = arc::Archive::instance().removeCommit(n->get_value<guint64>(cols.id)); !res) {
        reportError(res.error());
        return;
    }
    cursor = 0;
    if (auto r = refresh(); !r)
        reportError(r.error());
}


std::optional<uint64_t> get_cursor() {
    if (cursor > 0)
        return cursor;
    return {};
}
void set_cursor(uint64_t id) {
    cursor = id;
}
