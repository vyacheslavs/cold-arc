//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_UTILS_H
#define COLD_ARC_GTK_UTILS_H

#include <gtkmm-3.0/gtkmm.h>

template<typename T>
T* findWidget(const Glib::ustring& name, const Glib::RefPtr<Gtk::Builder>& builder) {
    T* widget = nullptr;
    builder->get_widget<T>(name, widget);
    if (!widget)
        throw std::runtime_error(Glib::ustring::compose("failed to find widget %1", name));
    return widget;
}

template<typename T>
T* findWidgetDerived(const Glib::ustring& name, const Glib::RefPtr<Gtk::Builder>& builder) {
    T* widget = nullptr;
    builder->get_widget_derived<T>(name, widget);
    if (!widget)
        throw std::runtime_error(Glib::ustring::compose("failed to find widget %1", name));
    return widget;
}

template<typename T>
T* findObject(const Glib::ustring& name, const Glib::RefPtr<Gtk::Builder>& builder) {
    auto widget = Glib::RefPtr<T>::cast_dynamic(builder->get_object(name));
    if (!widget)
        throw std::runtime_error(Glib::ustring::compose("failed to find widget %1", name));
    return widget.get();
}

template<typename DialogT, typename F>
void runDialog(const Glib::ustring& resourcePath, const Glib::ustring& dialogId, F callback) {
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource(resourcePath);
    std::unique_ptr<DialogT> _auto(findWidgetDerived<DialogT>(dialogId, builder));

    auto r = static_cast<Gtk::Dialog*>(_auto.get())->run();
    callback(_auto.get(), r);
}

template<typename T, typename Locker = std::mutex>
class BodyGuard {
public:

    class Access {
    public:
        T* operator->() {
            return get();
        }

        T* get() const {
            return &bodyref;
        }

        T& operator*() {
            return *get();
        }

    private:
        friend class BodyGuard;

        Access(T& _b, Locker& _l) : bodyref(_b), lockref(_l) { lockref.lock(); }

        T& bodyref;
        Locker& lockref;

    public:
        Access(const Access&) = delete;

        Access(Access&&) = delete;

        Access& operator=(const Access&) = delete;

        Access& operator=(Access&&) = delete;

        ~Access() { lockref.unlock(); }
    };

    template<typename ...Args>
    explicit BodyGuard(Args... args) :
            body(std::forward<Args>(args)...) {}

    Access access() {
        return {body, lock};
    }

private:
    T body;
    Locker lock;
};

bool endsWith(const Glib::ustring& filename, const Glib::ustring& postfix);

bool fileExists(const Glib::ustring& filename);

Glib::ustring generateSerial();

#endif //COLD_ARC_GTK_UTILS_H
