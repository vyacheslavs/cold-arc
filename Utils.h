//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_UTILS_H
#define COLD_ARC_GTK_UTILS_H

#include <gtkmm-3.0/gtkmm.h>
#include <tl/expected.hpp>
#include "assert.h"
#include "sqlite_modern_cpp.h"
#include <any>
#include <type_traits>

namespace cold_arc {
    enum class ErrorCode {
        None,
        ISOInitError,
        ISOPrepareImageError,
        ISONewFolderError,
        ISONewFileError,
        ISOBurnError,
        ExportDialogDBError,
        ExportDialogError,
        ISOOutputFileCreateError,
        ExportDialogBurnError,
        ArchiveConfigureDB,
        ArchiveInitError,
        ArchiveWrongDBVerionError,
        ArchiveCloneError,
        ArchiveOpenError,
        ArchiveThreadSafeModeError,
        NewArchiveError,
        NewArchiveExtractError,
        ExtractResourcesError,
        ExtractTargetCreateError,
        AddFontError,
        NewMediaError,
        CreateFolderError,
        CreatePathError,
        CreateFileError,
        WalkRootError,
        WalkRootComplexError,
        RemoveError,
        WalkTreeError,
        WalkTreeComplexError,
        BrowseError,
        BrowseMediaError,
        GetMediaForArcIdError,
        MediaConstructError,
        RemoveMediaError,
        MediaOccupyError,
        ConstructSettingsError,
        SettingsUpdateNameError,
        SettingsSwitchMediaError,
        BeginTransactionError,
        CommitTransactionError,
        RollbackTransactionError,
        UploadThreadError,
        RunDialogExError,
        NewFolderDialogError,
        UploadChooserDialogError,
        NewMediaDialogError,
        DeleteDialogError,
        ArchiveSettingsDialogError,
        UploadDialogError,
        MediaSelectError,
        MediaRemoveError,
    };

    typedef std::string(*explain_aux_callback)(const struct Error&);

    struct Error {
        ErrorCode code {ErrorCode::None};
        explain_aux_callback cb {nullptr};
        std::string fun_name;
        std::string source;
        int source_line {0};
        std::any aux;
    };

    template<typename T = void>
    using Result = tl::expected<T, Error>;

    using combined_error = std::pair<Error,Error>;

    std::string explain_nested_error(const Error& e);
    std::string explain_generic(const Error& e);
    std::string code_to_string(ErrorCode ec);
    std::string explain_invalid_parameter(const Error& e);
    std::string explain_sqlite_error(const Error& e);
    std::string explain_combined_error(const Error& e);
    [[maybe_unused]] Error make_combined_error_(const Error& e, const Error& might_be_e, ErrorCode c, const char* func, const char* source, int line);

    template<typename F>
    void unwind_nested(const Error& e, F && callback) {
        const Error* e_it = &e;
        while (e_it->cb == explain_nested_error) {
            callback(*e_it);
            e_it = &std::any_cast<const Error&>(e_it->aux);
        }
        callback(*e_it);
    }

    bool is_nested_error(const Error& e);

    template <int N>
    const Error& get_nested(const Error& e) {
        return std::any_cast<const Error&>(get_nested<N-1>(e).aux);
    }
    template <>
    const Error& get_nested<0>(const Error& e);
}

#define unexpected_error(code) tl::unexpected<cold_arc::Error>({code, cold_arc::explain_generic, __PRETTY_FUNCTION__, __FILE__, __LINE__})
#define unexpected_explained(code, foo, bar) tl::unexpected<cold_arc::Error>({code, foo, __PRETTY_FUNCTION__, __FILE__, __LINE__, bar})
#define unexpected_nested(code, nested) tl::unexpected<cold_arc::Error>({code, cold_arc::explain_nested_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, nested});
#define unexpected_invalid_input_parameter(code, param_name) tl::unexpected<cold_arc::Error>({code, cold_arc::explain_invalid_parameter, __PRETTY_FUNCTION__, __FILE__, __LINE__, std::string(param_name)})
#define unexpected_combined_error(code, nested1, nested2) tl::unexpected<cold_arc::Error>({code, cold_arc::explain_combined_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, cold_arc::combined_error(nested1, nested2)});
#define make_combined_error(e, might_e, c) cold_arc::make_combined_error_(e, might_e, c, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#define make_nested_error(c, nested) cold_arc::Error{c, cold_arc::explain_nested_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, nested}
#define make_sqlite_error(c, code) cold_arc::Error{c, cold_arc::explain_sqlite_error, __PRETTY_FUNCTION__, __FILE__, __LINE__, code}

template<typename T>
T* findWidget(const Glib::ustring& name, const Glib::RefPtr<Gtk::Builder>& builder) {
    T* widget = nullptr;
    builder->get_widget<T>(name, widget);
    if (!widget)
        throw std::runtime_error(Glib::ustring::compose("failed to find widget %1", name));
    return widget;
}

template<typename T, typename ... Args>
T* findWidgetDerived(const Glib::ustring& name, const Glib::RefPtr<Gtk::Builder>& builder, Args ... args) {
    T* widget = nullptr;
    builder->get_widget_derived<T>(name, widget, std::forward<Args>(args)...);
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

template <typename DialogT>
struct RunResult {
    std::unique_ptr<DialogT> dialog;
    int rc  {0};
};

namespace details {
    template <typename O, typename D, typename X = void>
    struct dialog_construct {
        template <typename ...Args>
        static O call(D& obj, const Glib::RefPtr<Gtk::Builder>& builder, Args...args) {
            return {};
        }
    };


    template <typename O, typename D>
    struct dialog_construct<O,D,typename std::enable_if_t<std::is_member_function_pointer_v<decltype(&D::construct)>> >  {
        template <typename ...Args>
        static O call(D& obj, const Glib::RefPtr<Gtk::Builder>& builder, Args...args) {
            return obj.construct(builder, std::forward<Args>(args)...);
        }
    };
}

template<typename DialogT, typename ...Args>
cold_arc::Result<RunResult<DialogT>> runDialog(const Glib::ustring& resourcePath, const Glib::ustring& dialogId, Args ... args) {
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource(resourcePath);
    std::unique_ptr<DialogT> _auto(findWidgetDerived<DialogT>(dialogId, builder));
    auto rv = details::dialog_construct<cold_arc::Result<>, DialogT>::template call(*_auto.get(), builder, std::forward<Args>(args)...);
    if (!rv)
        return unexpected_nested(cold_arc::ErrorCode::RunDialogExError, rv.error());
    auto r = static_cast<Gtk::Dialog*>(_auto.get())->run();
    return RunResult<DialogT>{std::move(_auto), r};
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

                Access(T& _b, Locker& _l)
                    : bodyref(_b),
                      lockref(_l) { lockref.lock(); }

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

/**
 * Extracts a blob from resource and outputs it into the file
 * @param resource_path
 * @param filename
 */
[[nodiscard]] cold_arc::Result<> extractFromResource(const Glib::ustring& resource_path, const Glib::ustring& filename);

[[nodiscard]] cold_arc::Result<> addFont(const Glib::ustring& font_resource);

enum class CalculateSHA256Errors {
    failedToOpenFileForReading,
    noSHA256CipherFound,
};

tl::expected<std::string, CalculateSHA256Errors> calculateSha256(const std::string& filename, uint64_t size, const std::function<void(uint64_t)>& callback);

struct HumanReadable {
        std::uintmax_t size{};
    private:
        friend
        std::ostream& operator<<(std::ostream& os, HumanReadable hr) {
            int i{};
            double mantissa = hr.size;
            for (; mantissa >= 1024.; mantissa /= 1024., ++i) {}
            mantissa = std::ceil(mantissa * 10.) / 10.;
            os << mantissa << " " << "BKMGTPE"[i];
            return i == 0 ? os : os << "B";
        }
};

void applyFontAwesome(Gtk::Widget* widget, bool resize = true);
void reportError(const cold_arc::Error& e, bool is_fatal = true);

template <typename F>
class ScopeExit {
    public:
        explicit ScopeExit(F && cb) : callback(std::forward<F>(cb)) {}
        ~ScopeExit() {
            callback();
        }
    private:
        F callback;
};

#endif //COLD_ARC_GTK_UTILS_H
