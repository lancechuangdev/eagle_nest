#include <iostream>
#include "main_window.h"
#include "app_paths.h"

std::string resolve_glade_file_path()
{
    const std::filesystem::path dev_path = "../ui.glade";
    const std::filesystem::path install_path = AppPaths::Install_Share_Dir / "ui.glade";

    if (std::filesystem::exists(dev_path))
    {
        return dev_path;
    }
    else if (std::filesystem::exists(install_path))
    {
        return install_path;
    }
    else
    {
        std::cerr << "UI file not found!" << std::endl;
        return "";
    }
}

int main(int argc, char **argv)
{
    auto app = Gtk::Application::create(argc, argv, "com.example.eagle_nest");
    auto builder = Gtk::Builder::create();

    try
    {
        auto glade_file_path = resolve_glade_file_path();
        if (glade_file_path.empty())
        {
            std::cerr << "Failed to resolve glade file path." << std::endl;
            return 1;
        }
        builder->add_from_file(glade_file_path);
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