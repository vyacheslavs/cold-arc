#include <iostream>
#include <gtkmm-3.0/gtkmm.h>
#include "MainWindow.h"
#include "Utils.h"
#include <openssl/evp.h>

int main(int argc, char** argv) {

    OpenSSL_add_all_digests();

    try {
        auto app = Gtk::Application::create(argc, argv, "org.coldarc");

        Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/main/glade.glade");
        builder->add_from_resource("/main/progress.glade");

        std::unique_ptr<MainWindow> win (findWidgetDerived<MainWindow>("main", builder));
        app->run(*win);

    } catch (const Glib::Exception& e) {
        std::cout << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << e.what()<<"\n";
    }
    return 0;
}
