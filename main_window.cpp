#include "main_window.h"
#include "app_paths.h"
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>
#include <fstream>

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder)
    : Gtk::Window(obj),
      m_builder(refBuilder)
{
    set_window_title("Eagle Nest");

    m_builder->get_widget("model_rbtn", m_model_btn);
    if (m_model_btn)
    {
        m_model_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("explore_rbtn", m_explore_btn);
    if (m_explore_btn)
    {
        m_explore_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("toolkit_rbtn", m_toolkit_btn);
    if (m_toolkit_btn)
    {
        m_toolkit_btn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    m_builder->get_widget("content_stack", m_content_stack);

    m_builder->get_widget("training_stack", m_training_stack);

    m_builder->get_widget("previous_btn", m_previous_btn);
    if (m_previous_btn)
    {
        m_previous_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_previous_clicked));
    }

    m_builder->get_widget("next_btn", m_next_btn);
    if (m_next_btn)
    {
        m_next_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_next_clicked));
    }

    m_builder->get_widget("training_step1_lbl", m_training_step1_lbl);
    if (m_training_step1_lbl)
    {
        m_training_step_labels.push_back(m_training_step1_lbl);
    }

    m_builder->get_widget("training_step2_lbl", m_training_step2_lbl);
    if (m_training_step2_lbl)
    {
        m_training_step_labels.push_back(m_training_step2_lbl);
    }

    m_builder->get_widget("training_step3_lbl", m_training_step3_lbl);
    if (m_training_step3_lbl)
    {
        m_training_step_labels.push_back(m_training_step3_lbl);
    }

    m_builder->get_widget("create_model_rbtn", m_create_model_rbtn);

    m_builder->get_widget("model_name_entry", m_model_name_entry);

    m_builder->get_widget("select_model_rbtn", m_select_model_rbtn);

    m_builder->get_widget("existing_models_cbox", m_existing_models_cbox);

    m_builder->get_widget("model_version_cbox", m_model_version_cbox);

    m_builder->get_widget("model_comment_tview", m_model_comment_tview);
        
    update_step_indicator();

    m_builder->get_widget("dataset_sources_refresh_btn", m_dataset_sources_refresh_btn);
    if (m_dataset_sources_refresh_btn)
    {
        m_dataset_sources_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_dataset_sources_refresh_clicked));
    }

    m_builder->get_widget("dataset_sources_lbox", m_dataset_sources_lbox);
    add_dataset_sources_header();
}

MainWindow::~MainWindow()
{
}

void MainWindow::set_window_title(const std::string &title)
{
    Gtk::Window *root;
    m_builder->get_widget("main_window", root);
    root->set_title(title);
}

void MainWindow::on_menu_toggled()
{
    if (m_model_btn->get_active())
    {
        m_content_stack->set_visible_child("page_model");
    }
    else if (m_explore_btn->get_active())
    {
        m_content_stack->set_visible_child("page_explore");
    }
    else if (m_toolkit_btn->get_active())
    {
        m_content_stack->set_visible_child("page_toolkit");
    }
}

void MainWindow::on_previous_clicked()
{
    if (m_current_step > 0) {
        --m_current_step;
        m_training_stack->set_visible_child(m_training_page_names[m_current_step]);
        m_next_btn->set_sensitive(true);
    }
    if (m_current_step == 0) {
        m_previous_btn->set_sensitive(false);
    }
    update_step_indicator();
    transition_step(false);
}

void MainWindow::on_next_clicked()
{
    if (m_current_step < m_training_page_names.size() - 1) {
        ++m_current_step;
        m_training_stack->set_visible_child(m_training_page_names[m_current_step]);
        m_previous_btn->set_sensitive(true);
    }
    if (m_current_step == m_training_page_names.size() - 1) {
        m_next_btn->set_sensitive(false);
    }
    update_step_indicator();
    transition_step(true);
}

void MainWindow::update_step_indicator() {
    for (size_t i = 0; i < m_training_step_labels.size(); ++i) {
        auto current_step_name = m_training_step_labels[i]->get_text();
        m_training_step_labels[i]->set_markup(i == m_current_step ? "<b><span foreground='blue'>" + current_step_name + "</span></b>" : current_step_name);
    }
}

void MainWindow::transition_step(bool step_forward)
{
    if (step_forward) // This function is triggered by the next button
    {
        if (m_current_step == 1) // Step 0 to Step 1
        {
            // Create training WIP directory if it doesn't exist
            if (!std::filesystem::exists(AppPaths::Training_WIP_Path))
            {
                std::filesystem::create_directories(AppPaths::Training_WIP_Path);
            }
            else // Clear the directory if it already exists
            {
                for (const auto &entry : std::filesystem::directory_iterator(AppPaths::Training_WIP_Path))
                {
                    std::filesystem::remove_all(entry.path());
                }
            }

            write_model_readme();
        }
    }
    else // This function is triggered by the previous button
    {
        if (m_current_step == 0) // Step 1 to Step 0
        {

        }
        else if (m_current_step == 1) // Step 2 to Step 1
        {
            
        }
        else
        {
            
        }
    }
}

void MainWindow::write_model_readme()
{
    std::string name;
    int version;
    std::string comment;

    if (m_create_model_rbtn->get_active())
    {
        name = m_model_name_entry->get_text();
        version = 1; // Default version
    }
    else if (m_select_model_rbtn->get_active())
    {
        name = m_existing_models_cbox->get_active_text();
        version = std::stoi(m_model_version_cbox->get_active_text()) + 1; // Increment version
    }
    else
    {
        std::cerr << "Not a valid option." << std::endl;
        return;
    }

    comment = m_model_comment_tview->get_buffer()->get_text();

    // Get current datetime in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_now), "%Y-%m-%dT%H:%M:%S");

    nlohmann::json readme_json;
    readme_json["name"] = name;
    readme_json["version"] = version;
    readme_json["comment"] = comment;
    readme_json["created_at"] = ss.str();

    std::ofstream out(AppPaths::Training_WIP_Path / "model.readme");
    out << std::setw(4) << readme_json << std::endl;
}

void MainWindow::on_dataset_sources_refresh_clicked()
{
    // Disable the button to prevent multiple clicks
    m_dataset_sources_refresh_btn->set_sensitive(false);

    // Launch detection in a separate thread
    std::thread([this]() {
        refresh_dataset_sources();

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect([this]() {
            m_dataset_sources_refresh_btn->set_sensitive(true);
            return false; // Disconnect idle handler
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::refresh_dataset_sources()
{
    remove_dataset_sources_except_header();
    add_dataset_source_row("Local Datasets", "Local", "~/eagle_eye/detection_projects");
    add_dataset_source_row("USB Datasets", "USB", "/path/to/usb");
    add_dataset_source_row("Remote Datasets", "Network", "192.168.1.2");
    update_all_datasource_connection_status();
}

void MainWindow::add_dataset_sources_header()
{
    auto header_grid = Gtk::make_managed<Gtk::Grid>();
    header_grid->set_margin_top(5);
    header_grid->set_margin_bottom(5);
    header_grid->set_margin_start(10);
    header_grid->set_margin_end(10);
    header_grid->set_column_homogeneous(true);

    auto name_label = Gtk::make_managed<Gtk::Label>("Name");
    name_label->set_halign(Gtk::Align::ALIGN_START);
    name_label->get_style_context()->add_class("heading");

    auto type_label = Gtk::make_managed<Gtk::Label>("Source Type");
    type_label->set_halign(Gtk::Align::ALIGN_START);
    type_label->get_style_context()->add_class("heading");

    auto info_label = Gtk::make_managed<Gtk::Label>("Connection Info");
    info_label->set_halign(Gtk::Align::ALIGN_START);
    info_label->get_style_context()->add_class("heading");

    auto status_label = Gtk::make_managed<Gtk::Label>("Connection Status");
    status_label->set_halign(Gtk::Align::ALIGN_START);
    status_label->get_style_context()->add_class("heading");

    // "Select All" checkbox
    auto select_all_cb = Gtk::make_managed<Gtk::CheckButton>();
    select_all_cb->set_tooltip_text("Select/Deselect All");
    select_all_cb->set_active(true);

    // Save checkbox pointer if you want to control all rows from it
    m_select_all_datasources_cbtn = select_all_cb;

    // Optional: connect signal to toggle all checkboxes
    m_select_all_datasources_cbtn->signal_toggled().connect([this]() {
        bool active = m_select_all_datasources_cbtn->get_active();
        for (auto* row : m_dataset_sources_lbox->get_children())
        {
            auto* list_row = dynamic_cast<Gtk::ListBoxRow*>(row);
            if (!list_row || !list_row->get_selectable()) continue;

            auto* grid = dynamic_cast<Gtk::Grid*>(list_row->get_child());
            if (!grid) continue;

            auto* cb = dynamic_cast<Gtk::CheckButton*>(grid->get_child_at(0, 0));
            if (cb) cb->set_active(active);
        }
    });

    header_grid->attach(*select_all_cb, 0, 0, 1, 1);
    header_grid->attach(*name_label, 1, 0, 1, 1);
    header_grid->attach(*type_label, 2, 0, 1, 1);
    header_grid->attach(*info_label, 3, 0, 1, 1);
    header_grid->attach(*status_label, 4, 0, 1, 1);

    // Insert at the top
    m_dataset_sources_lbox->append(*header_grid);
    m_dataset_sources_lbox->show_all_children();
}

void MainWindow::remove_dataset_sources_except_header()
{
    auto children = m_dataset_sources_lbox->get_children();
    for (size_t i = 1; i < children.size(); ++i)
    {
        auto* row = dynamic_cast<Gtk::ListBoxRow*>(children[i]);
        if (row) m_dataset_sources_lbox->remove(*row);
    }
}

void MainWindow::add_dataset_source_row(const std::string& name,
    const std::string& type,
    const std::string& connection_info,
    bool checked,
    const std::string& connection_status)
{
    // Create a new row and horizontal box
    auto row = Gtk::make_managed<Gtk::ListBoxRow>();
    auto grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_margin_top(5);
    grid->set_margin_bottom(5);
    grid->set_margin_start(10);
    grid->set_margin_end(10);
    grid->set_column_homogeneous(true);

    // Checkbox for selection
    auto checkbox = Gtk::make_managed<Gtk::CheckButton>();
    checkbox->set_hexpand(false);
    checkbox->set_halign(Gtk::Align::ALIGN_START);
    checkbox->set_active(checked);
    checkbox->signal_toggled().connect([this, checkbox, name]() {
        bool is_checked = checkbox->get_active();
        std::cout << "Source '" << name << "' toggled: " << (is_checked ? "Checked" : "Unchecked") << std::endl;
    
        // Optionally update your internal state here
    });

    // Source Name label
    auto name_label = Gtk::make_managed<Gtk::Label>(name);
    name_label->set_halign(Gtk::Align::ALIGN_START);

    // Source Type label (Local, USB, Network)
    auto type_label = Gtk::make_managed<Gtk::Label>(type);
    type_label->set_halign(Gtk::Align::ALIGN_START);

    // Connection Info label (IP address or path)
    auto info_label = Gtk::make_managed<Gtk::Label>(connection_info);
    info_label->set_halign(Gtk::Align::ALIGN_START);

    // Connection Status label (Connected, Disconnected, Unknown)
    auto status_label = Gtk::make_managed<Gtk::Label>(connection_status);
    status_label->set_halign(Gtk::Align::ALIGN_START);

    grid->attach(*checkbox, 0, 0, 1, 1);
    grid->attach(*name_label, 1, 0, 1, 1);
    grid->attach(*type_label, 2, 0, 1, 1);
    grid->attach(*info_label, 3, 0, 1, 1);
    grid->attach(*status_label, 4, 0, 1, 1);

    row->add(*grid);
    m_dataset_sources_lbox->append(*row);
    m_dataset_sources_lbox->show_all_children();
}

void MainWindow::update_all_datasource_connection_status()
{
    for (auto* child : m_dataset_sources_lbox->get_children())
    {
        auto* row = dynamic_cast<Gtk::ListBoxRow*>(child);
        if (!row) continue;

        auto* grid = dynamic_cast<Gtk::Grid*>(row->get_child());
        if (!grid) continue;

        // Get the Connection Info label (to be in column 3)
        auto* connection_info_label = dynamic_cast<Gtk::Label*>(grid->get_child_at(3, 0));
        if (!connection_info_label) continue;
        auto connection_info = connection_info_label->get_text();

        // Get the Connection Status label (to be in column 4)
        auto* status_label = dynamic_cast<Gtk::Label*>(grid->get_child_at(4, 0));
        if (!status_label) continue;

        auto connection_status = get_connection_status(connection_info);
        status_label->set_text(connection_status);
    }
}

std::string MainWindow::get_connection_status(const std::string& connection_info)
{
    return "Connected"; // Placeholder for actual connection status check
}

// void MainWindow::update_datasource_connection_status(const std::string& connection_info, const std::string& status_text)
// {
//     for (auto* child : m_dataset_sources_lbox->get_children())
//     {
//         auto* row = dynamic_cast<Gtk::ListBoxRow*>(child);
//         if (!row) continue;

//         auto* grid = dynamic_cast<Gtk::Grid*>(row->get_child());
//         if (!grid) continue;

//         // Get the Connection Info label (assumed to be in column 3)
//         auto* connection_info_label = dynamic_cast<Gtk::Label*>(grid->get_child_at(0, 3));
//         if (!connection_info_label) continue;

//         if (connection_info_label->get_text() == connection_info)
//         {
//             // Get the Connection Status label (assumed to be in column 4)
//             auto* status_label = dynamic_cast<Gtk::Label*>(grid->get_child_at(0, 4));
            
//             if (status_label)
//             {
//                 status_label->set_text(status_text);
//             }
//             break;
//         }
//     }
// }

