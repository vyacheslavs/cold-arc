//
// Created by developer on 9/20/23.
//

#ifndef COLD_ARC_GTK_HISTORYDIALOG_H
#define COLD_ARC_GTK_HISTORYDIALOG_H

#include <gtkmm-3.0/gtkmm.h>
#include "Utils.h"

class HistoryDialog : public Gtk::Dialog {
    public:
        HistoryDialog(Gtk::Dialog::BaseObjectType* win, const Glib::RefPtr<Gtk::Builder>& builder);

        [[nodiscard]] static cold_arc::Result<> run();
        [[nodiscard]] cold_arc::Result<> construct(const Glib::RefPtr<Gtk::Builder>& builder);

    private:

        void onApply();
        void onNewSP();
        void onRemove();

        [[nodiscard]] cold_arc::Result<> refresh();

        Gtk::TreeView* m_treeview {nullptr};
        Gtk::ListStore* m_liststore {nullptr};
        Gtk::Button* m_apply_button{nullptr};
        Gtk::Entry* m_new_point_desc{nullptr};
        Gtk::Button* m_new_point_btn{nullptr};
        Gtk::Button* m_remove_btn{nullptr};
};

[[maybe_unused]] std::optional<uint64_t> get_cursor();

void set_cursor(uint64_t);

#endif //COLD_ARC_GTK_HISTORYDIALOG_H
