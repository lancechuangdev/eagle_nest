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

    m_builder->get_widget("dataset_sources_grid", m_dataset_sources_grid);
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

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this]() {
            refresh_dataset_sources();
            m_dataset_sources_refresh_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::refresh_dataset_sources()
{
    clear_dataset_sources();
    add_dataset_sources_header();
    add_dataset_source_row(1, "Local Datasets", "Local", "~/eagle_eye/detection_projects");
    add_dataset_source_row(2, "USB Datasets", "USB", "/path/to/usb");
    add_dataset_source_row(3, "Remote Datasets", "Network", "192.168.1.2");
    update_all_datasource_connection_status();
}

void MainWindow::add_dataset_sources_header()
{
    int row = 0;

    // Checkbox
    auto checkbox = Gtk::make_managed<Gtk::CheckButton>();
    checkbox->set_halign(Gtk::Align::ALIGN_CENTER);
    checkbox->set_active(true);
    checkbox->signal_toggled().connect([this, checkbox]() {
        bool is_checked = checkbox->get_active();
        if (is_checked)
        {
            for (auto* cb : m_datasources_checkboxes)
            {
                cb->set_active(true);
            }
        }
        else
        {
            for (auto* cb : m_datasources_checkboxes)
            {
                cb->set_active(false);
            }
        }
        std::cout << "Select All toggled: " << (is_checked ? "Checked" : "Unchecked") << std::endl;
    });
    m_dataset_sources_grid->attach(*checkbox, 0, row, 1, 1);

    // Name label
    auto name_label = Gtk::make_managed<Gtk::Label>("Name");
    name_label->get_style_context()->add_class("heading");
    name_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*name_label, 1, row, 1, 1);

    // Type label
    auto type_label = Gtk::make_managed<Gtk::Label>("Source Type");
    type_label->get_style_context()->add_class("heading");
    type_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*type_label, 2, row, 1, 1);

    // Connection Info label
    auto info_label = Gtk::make_managed<Gtk::Label>("Connection Info");
    info_label->get_style_context()->add_class("heading");
    info_label->set_halign(Gtk::Align::ALIGN_START);
    info_label->set_hexpand(true);
    m_dataset_sources_grid->attach(*info_label, 3, row, 1, 1);

    // Connection Status label
    auto status_label = Gtk::make_managed<Gtk::Label>("Connection Status");
    status_label->get_style_context()->add_class("heading");
    status_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*status_label, 4, row, 1, 1);

    m_dataset_sources_grid->show_all_children();
}

void MainWindow::clear_dataset_sources()
{
    auto children = m_dataset_sources_grid->get_children();
    for (auto* child : children)
    {
        m_dataset_sources_grid->remove(*child);
    }
    m_dataset_sources_grid->show_all_children();
    m_datasources_checkboxes.clear();
    m_datasources_connections.clear();
}

void MainWindow::add_dataset_source_row(size_t row_index,
    const std::string& name,
    const std::string& type,
    const std::string& connection_info,
    bool checked,
    const std::string& connection_status)
{
    int row = row_index;

    // Checkbox
    auto checkbox = Gtk::make_managed<Gtk::CheckButton>();
    m_datasources_checkboxes.push_back(checkbox);
    checkbox->set_halign(Gtk::Align::ALIGN_CENTER);
    checkbox->set_active(checked);
    checkbox->signal_toggled().connect([this, checkbox, name]() {
        bool is_checked = checkbox->get_active();
        std::cout << "Source '" << name << "' toggled: " << (is_checked ? "Checked" : "Unchecked") << std::endl;
    
        // Optionally update your internal state here
    });
    m_dataset_sources_grid->attach(*checkbox, 0, row, 1, 1);

    // Name label
    auto name_label = Gtk::make_managed<Gtk::Label>(name);
    name_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*name_label, 1, row, 1, 1);

    // Type label
    auto type_label = Gtk::make_managed<Gtk::Label>(type);
    type_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*type_label, 2, row, 1, 1);

    // Connection Info label
    auto info_label = Gtk::make_managed<Gtk::Label>(connection_info);
    info_label->set_halign(Gtk::Align::ALIGN_START);
    info_label->set_hexpand(true);
    m_dataset_sources_grid->attach(*info_label, 3, row, 1, 1);

    // Connection Status label
    auto status_label = Gtk::make_managed<Gtk::Label>("Unknown");
    status_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*status_label, 4, row, 1, 1);

    // Store the connection info and status for later use
    m_datasources_connections.emplace_back(info_label, status_label);

    m_dataset_sources_grid->show_all_children();
}


void MainWindow::update_all_datasource_connection_status()
{
    for (const auto& [info_label, status_label] : m_datasources_connections) 
    {
        if (info_label && status_label)
        {
            auto connection_info = info_label->get_text();
            auto connection_status = get_connection_status(connection_info);
            status_label->set_text(connection_status);
        }
        else
        {
            std::cerr << "Error: Connection info or status label is null." << std::endl;
        }
    }
}

std::string MainWindow::get_connection_status(const std::string& connection_info)
{
    return "Connected"; // Placeholder for actual connection status check
}