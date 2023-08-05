#include <iostream>
#include <gtkmm-3.0/gtkmm.h>

std::unique_ptr<Gtk::Window>& getWindow();

int main(int argc, char** argv) {

    try {
        auto app = Gtk::Application::create(argc, argv, "org.coldarc");

        Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file("glade.glade");
        Gtk::Window *winPtr = nullptr;
        builder->get_widget("main", winPtr);

        if (!winPtr)
            return 1;

        getWindow().reset(winPtr);
        app->run(*getWindow());
    } catch (const Glib::Error& e) {
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