#include <iostream>
#include <gtkmm-3.0/gtkmm.h>
#include "MainWindow.h"
#include "Utils.h"
#include <openssl/evp.h>
#include "Signals.h"

int main(int argc, char** argv) {

    OpenSSL_add_all_digests();

    try {
        auto app = Gtk::Application::create(argc, argv, "org.coldarc");

        addFont("awesome-free-solid-900.otf");
        addFont("awesome-free-regular-400.otf");
        addFont("awesome-free-brands-regular-400.otf");

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
