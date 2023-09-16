#include <iostream>
#include <gtkmm-3.0/gtkmm.h>
#include "MainWindow.h"
#include "Utils.h"
#include <openssl/evp.h>
#include "Signals.h"
#include "Archive.h"

int main(int argc, char** argv) {

    OpenSSL_add_all_digests();

    try {
        auto app = Gtk::Application::create(argc, argv, "org.coldarc");

        if (auto res = addFont("awesome-free-solid-900.otf"); !res) {
            reportError(res.error());
            return 2;
        }
        if (auto res = addFont("awesome-free-regular-400.otf"); !res) {
            reportError(res.error());
            return 3;
        }
        if (auto res = addFont("awesome-free-brands-regular-400.otf"); !res) {
            reportError(res.error());
            return 4;
        }

        if (auto res = arc::Archive::configure(); !res) {
            reportError(res.error());
            return 1;
        }

        Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/main/glade.glade");

        Signals::instance().app_quit.connect(sigc::mem_fun(app.get(), &Gtk::Application::quit));
        std::unique_ptr<MainWindow> win (findWidgetDerived<MainWindow>("main", builder));

        app->run(*win);
    } catch (const Glib::Exception& e) {
        std::cout << "error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "error: " << e.what() << "\n";
    }
    return 0;
}
