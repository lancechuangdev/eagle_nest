#include <iostream>
#include "main_window.h"

int main(int argc, char **argv)
{
    auto app = Gtk::Application::create(argc, argv, "com.example.eagle_nest");
    auto builder = Gtk::Builder::create();

    try
    {
        auto gladeFile = "../ui.glade";
        builder->add_from_file(gladeFile);
    }
    catch (const Glib::FileError &ex)
    {
        std::cerr << "FileError: " << ex.what() << std::endl;
        // logger->log("FileError: " + std::string(ex.what()), Logger::ERROR);
        return 1;
    }
    catch (const Glib::MarkupError &ex)
    {
        std::cerr << "MarkupError: " << ex.what() << std::endl;
        // logger->log("MarkupError: " + std::string(ex.what()), Logger::ERROR);
        return 1;
    }
    catch (const Gtk::BuilderError &ex)
    {
        std::cerr << "BuilderError: " << ex.what() << std::endl;
        // logger->log("BuilderError: " + std::string(ex.what()), Logger::ERROR);
        return 1;
    }

    // Load top level window from glade.
    MainWindow *wnd = nullptr;
    builder->get_widget_derived("main_window", wnd);
    
    // Shows the window and returns when it is closed.
    int nRet = app->run(*wnd);

    return nRet;
}