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

    m_hide_button->signal_clicked().connect(sigc::mem_fun(this, &ProgressWindow::onHideButtonClicked));
    m_dispatcher.connect(sigc::mem_fun(this, &ProgressWindow::onThreadNotification));
    Signals::instance().upload_files.connect(sigc::mem_fun(this, &ProgressWindow::startUploadProcess));
    signal_delete_event().connect(sigc::mem_fun(this, &ProgressWindow::onCloseButtonClicked));
    m_current_action = findWidget<Gtk::ProgressBar>("current_action", builder);
    m_total_progress = findWidget<Gtk::ProgressBar>("total_progess", builder);
    m_current_path = findWidget<Gtk::Label>("current_path", builder);
}

void ProgressWindow::onThreadNotification() {
    // sending notification to GUI

    ProgressInfo progressCopy;
    {
        auto acc = m_progress_info.access();
        progressCopy = *acc;
        Signals::instance().upload_progress.emit(*acc);
    }

    m_current_path->set_text(progressCopy.info);
    m_total_progress->set_fraction(static_cast<double>(progressCopy.total_percent) / 100);

    if (progressCopy.action == ProgressInfo::Action::HASHING) {
        m_current_action->set_text("hashing ...");
        m_current_action->set_fraction(static_cast<double>(progressCopy.percent) / 100);
    }

    if (!progressCopy.upload_in_progress) {
        m_upload_thread->join();
        m_upload_thread.reset();

        std::cout << "STOPPED!\n";
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
}

void ProgressWindow::thread_main() {

    auto start_time = std::chrono::steady_clock::now();
    auto ready_to_report = [&start_time]() {
        auto now = std::chrono::steady_clock::now();
        auto ret = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count() > 0;
        if (ret)
            start_time = now;
        return ret;
    };
    {
        auto acc = m_progress_info.access();
        *acc = ProgressInfo{};
    }

    uint64_t index = 0;
    for (const auto& item : m_files) {
        {
            auto acc = m_progress_info.access();
            acc->total_percent = static_cast<int>(100* (index++) / m_files.size());
            acc->info = item->get_basename();
        }
        auto file_type = item->query_file_type(Gio::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);
        if (file_type == Gio::FILE_TYPE_REGULAR) {
            try {
                struct stat st;
                if (stat(item->get_path().c_str(), &st) < 0)
                    throw std::runtime_error("stat failed");
                UploadFileInfo up_info;
                up_info.sizeInBytes = st.st_size;
                up_info.dtOriginal = st.st_mtim.tv_sec;

                up_info.sha256hash = calculateSha256(item->get_path(), up_info.sizeInBytes, [&](uint64_t calculated) {
                    if (ready_to_report() && up_info.sizeInBytes > 0) {
                        {
                            auto acc = m_progress_info.access();
                            acc->upload_in_progress = true;
                            acc->action = ProgressInfo::Action::HASHING;
                            acc->percent = static_cast<int>(100 * calculated / up_info.sizeInBytes);
                        }
                        m_dispatcher.emit();
                    }
                });

                m_files_info.push_back(up_info);
            } catch (const std::exception&) {
                m_failed_files.push_back(item->get_path());
            }
        } else {
            m_failed_files.push_back(item->get_path());
        }
    }

    {
        auto acc = m_progress_info.access();
        acc->upload_in_progress = false;
    }
    m_dispatcher.emit();
}

std::string ProgressWindow::calculateSha256(const std::string& filename, uint64_t size, const std::function<void(uint64_t)>& callback) {
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
        callback(fp.tellg());
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

void ProgressWindow::tryShow() {
    if (!m_hidden)
        show();
}

void ProgressWindow::onHideButtonClicked() {
    m_hidden = true;
    hide();
}

void ProgressWindow::doShow() {
    m_hidden = false;
    show();
}

bool ProgressWindow::onCloseButtonClicked(GdkEventAny*) {
    onHideButtonClicked();
    return true;
}

