#include <iostream>
#include <gtkmm-3.0/gtkmm.h>
#include "MainWindow.h"

int main(int argc, char** argv) {

    try {
        auto app = Gtk::Application::create(argc, argv, "org.coldarc");

        Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/main/glade.glade");

        MainWindow* winPtr = nullptr;
        builder->get_widget_derived("main", winPtr);

        if (!winPtr)
            return 1;
        app->run(*winPtr);
    } catch (const Glib::Exception& e) {
        std::cout << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << e.what()<<"\n";
    }
    return 0;
}

std::unique_ptr<Gtk::Window>& getWindow() {
    static std::unique_ptr<Gtk::Window> win;
    return win;
}