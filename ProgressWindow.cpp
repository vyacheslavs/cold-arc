//
// Created by developer on 8/8/23.
//

#include <iostream>
#include "ProgressWindow.h"
#include "Utils.h"
#include "Signals.h"
#include <sys/stat.h>
#include <openssl/evp.h>
#include <fstream>
#include <iomanip>

ProgressWindow::ProgressWindow(Gtk::Window::BaseObjectType *win, const Glib::RefPtr<Gtk::Builder> &builder) : Gtk::Window(win) {
    m_hide_button = findWidget<Gtk::Button>("button_hide", builder);

    m_hide_button->signal_clicked().connect(sigc::mem_fun(this, &ProgressWindow::hide));
    m_dispatcher.connect(sigc::mem_fun(this, &ProgressWindow::on_thread_notification));
    Signals::instance().upload_files.connect(sigc::mem_fun(this, &ProgressWindow::startUploadProcess));
}

bool ProgressWindow::isUploadInProgress() const {
    return m_upload_in_progress.load(std::memory_order_acquire);
}

void ProgressWindow::on_thread_notification() {
    // sending notification to GUI

    {
        auto acc = m_progress_info.access();
        Signals::instance().upload_progress.emit(*acc);
    }

    if (!isUploadInProgress()) {
        m_upload_thread->join();
        m_upload_thread.reset();
    }
}

void ProgressWindow::startUploadProcess(const std::vector<Glib::RefPtr<Gio::File>> &files) {
    // copy files
    if (m_upload_thread) {
        std::cout << "another upload in progress\n";
        return;
    }

    m_files = files;
    m_files_info.clear();
    m_files_info.reserve(m_files.size());
    m_failed_files.clear();
    m_failed_files.reserve(m_files.size());
    m_upload_thread = std::make_unique<std::thread>(&ProgressWindow::thread_main, this);
    m_upload_in_progress.store(true, std::memory_order_release);
}

void ProgressWindow::thread_main() {

    for (const auto& item : m_files) {
        auto file_type = item->query_file_type(Gio::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);
        if (file_type == Gio::FILE_TYPE_REGULAR) {

            try {
                struct stat st;
                if (stat(item->get_path().c_str(), &st) < 0)
                    throw std::runtime_error("stat failed");
                UploadFileInfo up_info;
                up_info.sizeInBytes = st.st_size;
                up_info.dtOriginal = st.st_mtim.tv_sec;
                up_info.sha256hash = calculateSha256(item->get_path());

                std::cout << item->get_path() <<" : " << up_info.sha256hash<<"\n";

                m_files_info.push_back(up_info);
            } catch (const std::exception&) {
                m_failed_files.push_back(item->get_path());
            }
        }
    }

    m_upload_in_progress.store(false, std::memory_order_release);
    {
        auto acc = m_progress_info.access();
        acc->upload_in_progress = false;
    }
    m_dispatcher.emit();
}

std::string ProgressWindow::calculateSha256(const std::string& filename) {
    std::ifstream fp(filename, std::ios::in | std::ios::binary);
    if (not fp.good())
        throw std::runtime_error("failed to open file to calculate sha256 ");

    constexpr const std::size_t buffer_size { 1 << 12 };
    char buffer[buffer_size];
    unsigned char hash[EVP_MAX_MD_SIZE] = { 0 };
    unsigned int md_len = 0;

    auto md = EVP_get_digestbyname("sha256");

    auto _cleanup = [](EVP_MD_CTX* ptr){EVP_MD_CTX_destroy(ptr);};
    std::unique_ptr<EVP_MD_CTX, decltype(_cleanup)> mdctx(EVP_MD_CTX_create(), _cleanup);

    EVP_DigestInit_ex(mdctx.get(), md, NULL);

    while (fp.good()) {
        fp.read(buffer, buffer_size);
        EVP_DigestUpdate(mdctx.get(), buffer, fp.gcount());
    }
    EVP_DigestFinal_ex(mdctx.get(), hash, &md_len);

    std::ostringstream os;
    os << std::hex << std::setfill('0');

    for (int i = 0; i < md_len; ++i) {
        os << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return os.str();
}

