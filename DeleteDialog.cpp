//
// Created by developer on 8/17/23.
//

#include "DeleteDialog.h"
#include "Utils.h"
#include "Archive.h"
#include "Signals.h"

DeleteDialog::DeleteDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Dialog(win)
    {}

cold_arc::Result<> DeleteDialog::run(std::vector<uint64_t>&& items, std::set<uint64_t>&& medias) {

    auto r = runDialog<DeleteDialog>("/main/deletedlg.glade", "deletedlg", items, medias);
    if (!r)
        return unexpected_nested(cold_arc::ErrorCode::DeleteDialogError, r.error());

    if (r.value().rc != GTK_RESPONSE_OK)
        return {};

    if (arc::Archive::instance().settings->is_paranoic()) {
        std::stringstream ss;
        ss << "Save point before delete files";
        if (auto res = arc::Archive::instance().commit(ss.str()); !res)
            unexpected_nested(cold_arc::ErrorCode::DeleteDialogError, res.error());
    }

    if (auto res = arc::Archive::instance().beginTransaction(); !res)
        return unexpected_nested(cold_arc::ErrorCode::DeleteDialogError, res.error());

    auto ids = r.value().dialog->m_media_view->collectCheckedIds();
    auto excl = r.value().dialog->m_media_view->collectExclusions();

    for (const auto& id : r.value().dialog->items) {
        if (auto res = arc::Archive::instance().walkTree([&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk, sqlite3_uint64 dt, uint64_t parent_id) {
                if (auto res = arc::Archive::instance().remove(id, ids); !res)
                    return res.error();
                return cold_arc::Error{};
            }, id, excl, arc::Archive::CallbackLast | arc::Archive::WalkFilesAndFolders); !res) {
            auto rb = arc::Archive::instance().rollbackTransaction();
            return unexpected_combined_error(cold_arc::ErrorCode::DeleteDialogError, res.error(), rb);
        }
        if (auto res = arc::Archive::instance().remove(id, ids); !res) {
            auto rb = arc::Archive::instance().rollbackTransaction();
            return unexpected_combined_error(cold_arc::ErrorCode::DeleteDialogError, res.error(), rb);
        }
    }

    if (auto res = arc::Archive::instance().commitTransaction(); !res)
        return unexpected_nested(cold_arc::ErrorCode::DeleteDialogError, res.error());

    Signals::instance().update_main_window.emit();
    Signals::instance().update_tree.emit();
    Signals::instance().update_media_view.emit();
    return {};
}

cold_arc::Result<> DeleteDialog::construct(const Glib::RefPtr<Gtk::Builder>& builder, std::vector<uint64_t>&& _items, std::set<uint64_t>&& medias) {

    items = std::move(_items);
    m_medias = std::move(medias);
    m_media_view = findWidgetDerived<MediaView>("mediaview", builder, false);
    for (const auto& m : m_medias) {
        auto mp = arc::Archive::instance().settings->media()->construct(m);
        if (!mp)
            return unexpected_nested(cold_arc::ErrorCode::DeleteDialogError, mp.error());

        if (!mp.value()->locked())
            m_media_view->addMedia(mp.value()->id(), mp.value()->capacity(), mp.value()->occupied(), mp.value()->name(), mp.value()->serial(), mp.value()->locked());
    }
    return {};
}
