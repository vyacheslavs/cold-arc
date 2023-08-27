//
// Created by developer on 8/17/23.
//

#include "DeleteDialog.h"
#include "Utils.h"
#include "Archive.h"
#include "Signals.h"

DeleteDialog::DeleteDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder, std::vector<uint64_t>&& _items, std::set<uint64_t>&& medias)
    : Gtk::Dialog(win),
      items(std::move(_items)),
      m_medias(std::move(medias))
    {

    m_media_view = findWidgetDerived<MediaView>("mediaview", builder, false);
    for (const auto& m : m_medias) {
        auto mp = arc::Archive::instance().settings->media()->getMedia(m);
        m_media_view->addMedia(mp->id(), mp->capacity(), mp->occupied(), mp->name(), mp->serial());
    }
}

int DeleteDialog::run(std::vector<uint64_t>&& items, std::set<uint64_t>&& medias) {
    runDialog<DeleteDialog>("/main/deletedlg.glade", "deletedlg", [](DeleteDialog* dlg, int rc) {
        auto ids = dlg->m_media_view->collectCheckedIds();
        auto excl = dlg->m_media_view->collectExclusions();
        for (const auto& id : dlg->items) {
            arc::Archive::instance().walkTree([&](sqlite3_uint64 id, const std::string& typ, const std::string& name, const std::string& hash, const std::string& lnk, sqlite3_uint64 dt, uint64_t parent_id) {
                arc::Archive::instance().remove(id, ids);
            }, id, excl, arc::Archive::CallbackLast | arc::Archive::WalkFilesAndFolders);
            arc::Archive::instance().remove(id, ids);
        }
        Signals::instance().update_main_window.emit();
        Signals::instance().update_tree.emit();
        Signals::instance().update_media_view.emit();
    }, std::move(items), std::move(medias));
    return 0;
}
