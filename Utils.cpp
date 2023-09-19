//
// Created by developer on 8/7/23.
//

#include "Utils.h"
#include "Signals.h"
#include <sys/stat.h>
#include <random>
#include <fstream>
#include <filesystem>
#include <openssl/evp.h>
#include "ErrorBox.h"

bool endsWith(const Glib::ustring& filename, const Glib::ustring& postfix) {
    if (postfix.size() > filename.size()) return false;
    return std::equal(postfix.rbegin(), postfix.rend(), filename.rbegin());
}

bool fileExists(const Glib::ustring& filename) {
    struct stat st{};
    return stat(filename.c_str(), &st) == 0;
}

Glib::ustring generateSerial() {
    auto length = 16;
    static auto& chrs = "0123456789"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(length);

    while (length--)
        s += chrs[pick(rg)];

    return s;
}

static std::string explain_extract_error(const cold_arc::Error& e) {
    std::stringstream ss;
    ss << cold_arc::explain_generic(e) << "failed to extract skeleton database to "<<std::any_cast<std::string>(e.aux)<<"\n";
    return ss.str();
}

cold_arc::Result<> extractFromResource(const Glib::ustring& resource_path, const Glib::ustring& filename) {

    if (resource_path.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ExtractResourcesError, "resource_path");
    if (filename.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ExtractResourcesError, "filename");

    Glib::RefPtr<const Glib::Bytes> blob;
    try {
        blob = Gio::Resource::lookup_data_global(resource_path);
        if (!blob)
            return unexpected_error(cold_arc::ErrorCode::ExtractResourcesError);
    } catch (const Gio::Error& e) {
        return unexpected_error(cold_arc::ErrorCode::ExtractResourcesError);
    }

    {
        std::ofstream of(filename.c_str());
        if (!of)
            return unexpected_explained(cold_arc::ErrorCode::ExtractTargetCreateError, explain_extract_error, std::string(filename));
        gsize sz;
        const char* buf = reinterpret_cast<const char*>(blob->get_data(sz));
        if (!buf)
            return unexpected_error(cold_arc::ErrorCode::ExtractResourcesError);
        of.write(buf, static_cast<std::streamsize>(sz));
    }
    return {};
}

static std::string explain_add_font_error(const cold_arc::Error& e) {
    std::stringstream ss;
    ss << cold_arc::explain_generic(e) << "failed create temporary directory, os error: "<<std::any_cast<int>(e.aux)<<"\n";
    return ss.str();
}

cold_arc::Result<> addFont(const Glib::ustring& font_resource) {
    if (font_resource.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::AddFontError, "font_resource");
    const auto tmpdir = std::string("/tmp/.cold-arc");
    std::error_code ec;
    std::filesystem::create_directories(tmpdir, ec);

    if (ec.value() != 0)
        return unexpected_explained(cold_arc::ErrorCode::AddFontError, explain_add_font_error, ec.value());

    const auto res = "/main/" + font_resource;
    const auto out = tmpdir + "/" + font_resource;

    if (auto ret = extractFromResource(res, out); !ret)
        return unexpected_nested(cold_arc::ErrorCode::AddFontError, ret.error());

    auto fc_config = FcConfigGetCurrent();
    if (!FcConfigAppFontAddFile(fc_config, (const FcChar8*) out.c_str()))
        return unexpected_error(cold_arc::ErrorCode::AddFontError);

    return {};
}

cold_arc::Result<std::string> sha256(const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE] = {0};
    unsigned int md_len = 0;

    auto md = EVP_get_digestbyname("sha256");
    if (!md)
        return unexpected_error(cold_arc::ErrorCode::SHA256CalcError);

    auto _cleanup = [](EVP_MD_CTX* ptr) { EVP_MD_CTX_destroy(ptr); };
    std::unique_ptr<EVP_MD_CTX, decltype(_cleanup)> mdctx(EVP_MD_CTX_create(), _cleanup);

    EVP_DigestInit_ex(mdctx.get(), md, nullptr);
    EVP_DigestUpdate(mdctx.get(), data.data(), data.size());
    EVP_DigestFinal_ex(mdctx.get(), hash, &md_len);

    std::ostringstream os;
    os << std::hex << std::setfill('0');

    for (int i = 0; i < md_len; ++i) {
        os << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return os.str();
}

tl::expected<std::string, CalculateSHA256Errors> calculateSha256(const std::string& filename, uint64_t size, const std::function<void(uint64_t)>& callback) {
    std::ifstream fp(filename, std::ios::in | std::ios::binary);
    if (not fp.good())
        return tl::unexpected(CalculateSHA256Errors::failedToOpenFileForReading);

    constexpr const std::size_t buffer_size{1 << 12};
    char buffer[buffer_size];
    unsigned char hash[EVP_MAX_MD_SIZE] = {0};
    unsigned int md_len = 0;

    auto md = EVP_get_digestbyname("sha256");
    if (!md)
        return tl::unexpected(CalculateSHA256Errors::noSHA256CipherFound);

    auto _cleanup = [](EVP_MD_CTX* ptr) { EVP_MD_CTX_destroy(ptr); };
    std::unique_ptr<EVP_MD_CTX, decltype(_cleanup)> mdctx(EVP_MD_CTX_create(), _cleanup);

    EVP_DigestInit_ex(mdctx.get(), md, nullptr);

    uint64_t fraction = 0;
    while (fp.good()) {
        fp.read(buffer, buffer_size);
        callback(fraction);
        EVP_DigestUpdate(mdctx.get(), buffer, fp.gcount());
        fraction += fp.gcount();
    }
    EVP_DigestFinal_ex(mdctx.get(), hash, &md_len);

    std::ostringstream os;
    os << std::hex << std::setfill('0');

    for (int i = 0; i < md_len; ++i) {
        os << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return os.str();
}

void applyFontAwesome(Gtk::Widget* widget, bool resize) {
    auto desc = widget->get_pango_context()->get_font_description();
    desc.set_family("Font Awesome 6 Free");
    if (resize)
        desc.set_size(18 * Pango::SCALE);
    desc.set_weight(Pango::WEIGHT_HEAVY);
    widget->get_pango_context()->set_font_description(desc);
}

void reportError(const cold_arc::Error& e, bool is_fatal) {
    ErrorBox::run(e);
    if (is_fatal)
        Signals::instance().app_quit.emit();
}

std::string cold_arc::explain_nested_error(const Error& e) {
    std::stringstream ss;
    unwind_nested(e, [&](const Error& ex) {
        if (is_nested_error(ex))
            ss << ex.fun_name << " ("<<ex.source<<":"<<ex.source_line<<")\n";
        else if (ex.cb)
            ss << ex.cb(ex);
        else
            ss << explain_generic(ex) << "\n";
    });
    return ss.str();
}

std::string cold_arc::explain_generic(const Error& ex) {
    std::stringstream ss;
    ss << ex.fun_name << " ("<<ex.source<<":"<<ex.source_line<<") "<<code_to_string(ex.code);
    return ss.str();
}

std::string cold_arc::explain_invalid_parameter(const Error& e) {
    std::stringstream ss;
    ss << explain_generic(e) << "invalid input parameter `"<<std::any_cast<std::string>(e)<<"`\n";
    return ss.str();
}

std::string cold_arc::explain_sqlite_error(const Error& e) {
    std::stringstream ss;
    ss << explain_generic(e) << "sqlite error code: " << std::any_cast<int>(e.aux) << "\n";
    return ss.str();
}

bool cold_arc::is_nested_error(const Error& e) {
    return e.cb == explain_nested_error;
}

namespace cold_arc {

    template<>
    const Error& get_nested<0>(const Error& e) {
        return e;
    }

}

std::string cold_arc::code_to_string(ErrorCode ec) {
    static const std::unordered_map<ErrorCode, std::string> error_map {
        {ErrorCode::None, "No error"}
    };

    auto it = error_map.find(ec);
    if (it != error_map.end())
        return it->second;

    return "";
}
std::string cold_arc::explain_combined_error(const cold_arc::Error& e) {
    std::stringstream ss;

    auto combined = std::any_cast<combined_error>(e.aux);
    ss << explain_nested_error(combined.first);
    ss << "\n at the same time:\n";
    ss << explain_nested_error(combined.second);

    return ss.str();
}
