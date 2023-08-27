//
// Created by developer on 8/17/23.
//

#ifndef COLD_ARC_GTK_MEDIAVIEW_H
#define COLD_ARC_GTK_MEDIAVIEW_H

#include <gtkmm-3.0/gtkmm.h>

class MediaView : public Gtk::TreeView {
    public:
        MediaView(Gtk::TreeView::BaseObjectType* bot, const Glib::RefPtr<Gtk::Builder>& builder, bool toolbar = true);
        void update();
        void onNewMediaButtonClicked();
        void addMedia(uint64_t id, uint64_t capacity, uint64_t occupied, const std::string& name, const std::string& serial);
        std::string collectExclusions() const;
        std::vector<uint64_t> collectCheckedIds() const;

        sigc::signal<void> mediaToggled;
    private:

        Gtk::TreeView* m_media_view;
        Gtk::ListStore* m_media_store;
        Gtk::Toolbar* m_media_toolbar;
        Gtk::TreeSelection* m_media_view_selection;
        Gtk::ToolButton* m_media_view_select_button;
        Gtk::ToolButton* m_media_view_remove_button;
        Gtk::ToolButton* m_media_new_button;

        void onMediaToggle(const Glib::ustring& path);
        void onMediaViewSelectionChanged();
        void onMediaViewSelectButton();
        void onMediaViewRemoveButtonClicked();
};


#endif //COLD_ARC_GTK_MEDIAVIEW_H
