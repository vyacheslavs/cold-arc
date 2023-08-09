//
// Created by developer on 8/7/23.
//

#include "Utils.h"
#include <sys/stat.h>
#include <random>
#include <fstream>
#include <filesystem>
#include <openssl/evp.h>

bool endsWith(const Glib::ustring &filename, const Glib::ustring &postfix) {
    if (postfix.size() > filename.size()) return false;
    return std::equal(postfix.rbegin(), postfix.rend(), filename.rbegin());
}

bool fileExists(const Glib::ustring &filename) {
    struct stat st {};
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

    while(length--)
        s += chrs[pick(rg)];

    return s;
}

void extractFromResource(const Glib::ustring& resource_path, const Glib::ustring& filename) {
    Glib::RefPtr< const Glib::Bytes > blob = Gio::Resource::lookup_data_global(resource_path);
    {
        std::ofstream of(filename.c_str());
        if (!of)
            throw std::runtime_error("failed to extract resource");
        gsize sz;
        const char* buf = reinterpret_cast<const char*>(blob->get_data(sz));
        of.write(buf, static_cast<std::streamsize>(sz));
    }
}

void addFont(const Glib::ustring& font_resource) {

    const auto tmpdir = std::string("/tmp/.cold-arc");
    std::filesystem::create_directories(tmpdir);
    const auto res = "/main/" + font_resource;
    const auto out = tmpdir + "/" + font_resource;

    extractFromResource(res, out);
    auto fc_config = FcConfigGetCurrent();
    if (!FcConfigAppFontAddFile(fc_config, (const FcChar8 *)out.c_str())) {
        throw std::runtime_error("Failed to load icon font");
    }
}


tl::expected<std::string, CalculateSHA256Errors> calculateSha256(const std::string& filename, uint64_t size, const std::function<void(uint64_t)>& callback) {
    std::ifstream fp(filename, std::ios::in | std::ios::binary);
    if (not fp.good())
        return tl::unexpected(CalculateSHA256Errors::failedToOpenFileForReading);

    constexpr const std::size_t buffer_size { 1 << 12 };
    char buffer[buffer_size];
    unsigned char hash[EVP_MAX_MD_SIZE] = { 0 };
    unsigned int md_len = 0;

    auto md = EVP_get_digestbyname("sha256");
    if (!md)
        return tl::unexpected(CalculateSHA256Errors::noSHA256CipherFound);

    auto _cleanup = [](EVP_MD_CTX* ptr){EVP_MD_CTX_destroy(ptr);};
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
