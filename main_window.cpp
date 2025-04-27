#include "main_window.h"
#include "app_paths.h"
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>
#include <fstream>

namespace fs = std::filesystem;
using DatasetSource = MainWindow::DatasetSource;
using json = nlohmann::json;

static bool is_image_file(const fs::path& p)
{
    static const std::set<std::string> exts = {".png", ".jpg", ".jpeg", ".bmp"};
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return exts.count(ext) > 0;
}

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

    m_builder->get_widget("explorer_stack", m_explorer_stack);

    m_builder->get_widget("explorer_dataset_sources_rbtn", m_explorer_dataset_sources_rbtn);
    if (m_explorer_dataset_sources_rbtn)
    {
        m_explorer_dataset_sources_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_explorer_toggled));
    }

    m_builder->get_widget("explorer_images_from_sources_rbtn", m_explorer_images_from_sources_rbtn);
    if (m_explorer_images_from_sources_rbtn)
    {
        m_explorer_images_from_sources_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_explorer_toggled));
    }

    m_builder->get_widget("explorer_training_images_rbtn", m_explorer_training_images_rbtn);
    if (m_explorer_training_images_rbtn)
    {
        m_explorer_training_images_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_explorer_toggled));
    }

    m_builder->get_widget("dataset_sources_refresh_btn", m_dataset_sources_refresh_btn);
    if (m_dataset_sources_refresh_btn)
    {
        m_dataset_sources_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_dataset_sources_refresh_clicked));
    }

    m_builder->get_widget("dataset_sources_grid", m_dataset_sources_grid);
    add_dataset_sources_header();

    m_builder->get_widget("dataset_sources_cbox", m_dataset_sources_cbox);
    m_dataset_sources_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_dataset_source_changed));

    m_builder->get_widget("image_category_cbox", m_image_category_cbox);

    m_builder->get_widget("images_from_sources_refresh_btn", m_images_from_sources_refresh_btn);
    if (m_images_from_sources_refresh_btn)
    {
        m_images_from_sources_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_images_from_sources_refresh_clicked));
    }

    m_builder->get_widget("explorer_images_lbox", m_explorer_images_lbox);
    // Handle row selection
    m_explorer_images_lbox->signal_row_activated().connect([this](Gtk::ListBoxRow* row) {
        if (row)
        {
            auto path_ptr = static_cast<fs::path*>(row->get_data("image_path"));
            if (path_ptr)
            {
                const fs::path& image_path = *path_ptr;
                // on_img_row_clicked(image_path);
                std::cout << "Row activated for image: " << image_path.string() << std::endl;
            }
        }
    });
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

void MainWindow::on_explorer_toggled()
{
    if (m_explorer_dataset_sources_rbtn->get_active())
    {
        m_explorer_stack->set_visible_child("page_dataset_souces");
    }
    else if (m_explorer_images_from_sources_rbtn->get_active())
    {
        m_explorer_stack->set_visible_child("page_images_from_sources");
    }
    else if (m_explorer_training_images_rbtn->get_active())
    {
        m_explorer_stack->set_visible_child("page_training_images");
    }
    else
    {
        std::cerr << "No valid option selected." << std::endl;
    }
}

void MainWindow::on_dataset_source_changed()
{
    auto id = m_dataset_sources_cbox->get_active_id();

    if (id == "separator")
    {
        // Ignore separator selection
        m_dataset_sources_cbox->set_active(-1);
        return;
    }

    // Handle valid selection
    std::cout << "Selected ID: " << id << std::endl;
}

void MainWindow::on_dataset_sources_refresh_clicked()
{
    // Disable the button to prevent multiple clicks
    m_dataset_sources_refresh_btn->set_sensitive(false);

    // Launch detection in a separate thread
    std::thread([this]() {
        discover_dataset_sources();

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this]() {
            clear_dataset_sources();
            add_dataset_sources_header();
            for (const auto& [id, name, type, path, status] : m_dataset_sources)
            {
                if (id <= 0)
                {
                    std::cerr << "Row index must be non-negative." << std::endl;
                    continue;
                }

                // Add each dataset source to the grid
                add_dataset_source_row(id, name, type, path, status);
            }
            
            m_dataset_sources_refresh_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::discover_dataset_sources()
{
    size_t datasource_id = 1; // Starting ID for dataset sources

    // Clear previous dataset sources
    m_dataset_sources.clear();

    // look for datasets in the following locations:
    // 1. Local Datasets
    // 2. USB Datasets (future)
    // 3. Remote Datasets (future)

    // Local Datasets
    add_local_dataset_source(datasource_id++);

    // sources.emplace_back(DatasetSource{
    //     2,
    //     "USB Datasets",
    //     "USB",
    //     "/path/to/usb",
    //     "Connected"
    // });

    // sources.emplace_back(DatasetSource{
    //     3,
    //     "Remote Datasets",
    //     "Network",
    //     "192.168.1.2",
    //     "Connected"
    // });
}

void MainWindow::add_local_dataset_source(size_t datasource_display_id)
{
    auto base_path = AppPaths::Detection_Projects_Path;
    bool is_local_datasets_found = false;

    if (!fs::exists(base_path))
    {
        std::cerr << "Detection Projects path does not exist: " << base_path << std::endl;
        return;
    }

    for (const auto& entry : fs::directory_iterator(base_path))
    {
        if (entry.is_directory())
        {
            auto dataset_path = entry.path() / "dataset";
            auto project_name = entry.path().filename().string();
            if (fs::exists(dataset_path))
            {
                is_local_datasets_found = true;
                m_dataset_sources.emplace_back(DatasetSource{
                    -1, // should not appear in the UI
                    project_name,
                    "Local",
                    dataset_path.string(),
                    "Connected"
                });
            }
        }
    }

    if (is_local_datasets_found)
    {
        m_dataset_sources.emplace_back(DatasetSource{
            datasource_display_id,
            "Local Datasets",
            "Local",
            base_path.string(),
            "Connected"
        });
    }
    else
    {
        std::cout << "No local datasets found in: " << base_path << std::endl;
    }
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
    const std::string& connection_status,
    bool checked)
{
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
    m_dataset_sources_grid->attach(*checkbox, 0, row_index, 1, 1);

    // Name label
    auto name_label = Gtk::make_managed<Gtk::Label>(name);
    name_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*name_label, 1, row_index, 1, 1);

    // Type label
    auto type_label = Gtk::make_managed<Gtk::Label>(type);
    type_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*type_label, 2, row_index, 1, 1);

    // Connection Info label
    auto info_label = Gtk::make_managed<Gtk::Label>(connection_info);
    info_label->set_halign(Gtk::Align::ALIGN_START);
    info_label->set_hexpand(true);
    m_dataset_sources_grid->attach(*info_label, 3, row_index, 1, 1);

    // Connection Status label
    auto status_label = Gtk::make_managed<Gtk::Label>(connection_status);
    status_label->set_halign(Gtk::Align::ALIGN_START);
    m_dataset_sources_grid->attach(*status_label, 4, row_index, 1, 1);

    // Store the connection info and status for later use
    m_connection_status_labels.emplace_back(status_label);
    m_datasources_connections.emplace_back(info_label, status_label);

    m_dataset_sources_grid->show_all_children();
}

void MainWindow::on_images_from_sources_refresh_clicked()
{
    // Disable the button to prevent multiple clicks
    m_images_from_sources_refresh_btn->set_sensitive(false);

    refresh_dataset_sources_options();
    refresh_image_category_options();

    // Launch detection in a separate thread
    std::thread([this]() {
        std::vector<fs::path> images;
        // Create a JSON object to store the image info
        json image_info_json;

        try
        {
            for (const auto& source : m_dataset_sources)
            {
                // Skip the data source that only for display purpose.
                if (source.dispaly_index >= 0)
                {
                    continue;
                }
        
                auto normal_dataset_path = fs::path(source.connection_info) / "normal";
                auto abnormal_dataset_path = fs::path(source.connection_info) / "abnormal";

                if (fs::exists(normal_dataset_path))
                {
                    for (auto const& dir_entry :
                         fs::recursive_directory_iterator(normal_dataset_path))
                    {
                        if (dir_entry.is_regular_file() &&
                            is_image_file(dir_entry.path()))
                        {
                            images.emplace_back(dir_entry.path());

                            json image_entry;
                            image_entry["file_path"] = dir_entry.path().string();
                            image_entry["source_name"] = source.name;
                            image_entry["source_type"] = source.type;
                            
                            // Add the image entry to the JSON array
                            image_info_json.push_back(image_entry);
                        }
                    }
                }

                if (fs::exists(abnormal_dataset_path))
                {
                    for (auto const& dir_entry :
                         fs::recursive_directory_iterator(abnormal_dataset_path))
                    {
                        if (dir_entry.is_regular_file() &&
                            is_image_file(dir_entry.path()))
                        {
                            images.emplace_back(dir_entry.path());

                            json image_entry;
                            image_entry["file_path"] = dir_entry.path().string();
                            image_entry["source_name"] = source.name;
                            image_entry["source_type"] = source.type;
                            
                            // Add the image entry to the JSON array
                            image_info_json.push_back(image_entry);
                        }
                    }
                }
            }

            // Open the file to write the JSON object
            auto json_file_path = AppPaths::Dataset_Images_Path / "dataset.json";
            std::ofstream json_file(json_file_path);
            if (json_file.is_open())
            {
                json_file << std::setw(4) << image_info_json << std::endl; // Pretty print with indentations
                json_file.close();
                std::cout << "Image info written to " << json_file_path << std::endl;
            }
            else
            {
                std::cerr << "Failed to open file: " << json_file_path << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error while scanning images: " << e.what() << '\n';
        }

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this, imgs = std::move(images)]() {
            populate_explorer_images_listbox(imgs);
            m_images_from_sources_refresh_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::refresh_dataset_sources_options()
{
    // Clear the existing options
    m_dataset_sources_cbox->remove_all();

    // Add the "All" option at the top
    m_dataset_sources_cbox->append("all", "All Connected Sources");

    // Add new options
    for (const auto& source : m_dataset_sources)
    {
        // Skip the data source that should not be displayed in the UI.
        if (source.dispaly_index <= 0)
        {
            continue;
        }

        // Add the dataset source to the combo box
        m_dataset_sources_cbox->append(source.name, source.name);
    }

    // Add a separator, not a real option.
    m_dataset_sources_cbox->append("separator", "──────────────────");

    // Add the "Training Set" option at the bottom
    m_dataset_sources_cbox->append("training", "Training Set");

    // Set the first option as active
    m_dataset_sources_cbox->set_active(0);

    // Show the updated options
    m_dataset_sources_cbox->show();
}

void MainWindow::refresh_image_category_options()
{
    // Clear the existing options
    m_image_category_cbox->remove_all();

    // Add all options
    m_image_category_cbox->append("All");
    m_image_category_cbox->append("Normal");
    m_image_category_cbox->append("Abnormal");

    // Set the first option as active
    m_image_category_cbox->set_active(0);
    
    // Show the updated options
    m_image_category_cbox->show();
}

void MainWindow::populate_explorer_images_listbox(const std::vector<fs::path>& images)
{
    // clear previous rows
    for (auto* child : m_explorer_images_lbox->get_children())
    m_explorer_images_lbox->remove(*child);

    // add one row per image
    for (const auto& p : images)
    {
        auto filename = p.filename().string();
        auto hbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);

        // Label for the image
        auto lbl  = Gtk::make_managed<Gtk::Label>(filename);
        lbl->set_xalign(0);
        lbl->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        lbl->set_tooltip_text(filename);
        lbl->set_max_width_chars(40); // You can tweak this as needed
        lbl->set_single_line_mode(true); // Prevent wrapping

        // Action button
        auto btn = Gtk::make_managed<Gtk::Button>("Add");
        btn->set_valign(Gtk::ALIGN_CENTER);
        btn->signal_clicked().connect([this, btn, p]()
        {
            if (btn->get_label() == "Add")
            {
                on_img_add_clicked(p);
                btn->set_label("Remove");
            }
            else if (btn->get_label() == "Remove")
            {
                on_img_remove_clicked(p);
                btn->set_label("Add");
            }
        });

        // Pack them into the hbox
        hbox->pack_start(*lbl, Gtk::PACK_EXPAND_WIDGET);
        hbox->pack_start(*btn, Gtk::PACK_SHRINK);

        // Add the hbox to a row
        auto row  = Gtk::make_managed<Gtk::ListBoxRow>();
        row->add(*hbox);

        // Store the path as custom data
        row->set_data("image_path", new fs::path(p));

        // Add the row to the listbox
        m_explorer_images_lbox->append(*row);
    }
    m_explorer_images_lbox->show_all_children();
}

void MainWindow::on_img_add_clicked(const fs::path& image_path)
{
    // Do something with the path
    std::cout << "Add button clicked for: " << image_path.string() << std::endl;
    add_image_to_dataset(image_path, "normal");
}

void MainWindow::on_img_remove_clicked(const fs::path& image_path)
{
    // Do something with the path
    std::cout << "Remove button clicked for: " << image_path.string() << std::endl;
    remove_image_from_dataset(image_path, "normal");
}

void MainWindow::add_image_to_dataset(const fs::path& src_path, const std::string& category)
{
    try
    {
        auto dataset_path = AppPaths::Dataset_Images_Path / category;
        if (!fs::exists(dataset_path))
        {
            fs::create_directories(dataset_path);
        }

        auto dest_path = dataset_path / src_path.filename();
        fs::copy(src_path, dest_path, fs::copy_options::overwrite_existing);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to add image: " << ex.what() << std::endl;
    }
}

void MainWindow::remove_image_from_dataset(const fs::path& src_path, const std::string& category)
{
    try
    {
        auto dataset_path = AppPaths::Dataset_Images_Path / category;
        auto dest_path = dataset_path / src_path.filename();
        if (fs::exists(dest_path))
        {
            fs::remove(dest_path);
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to remove image: " << ex.what() << std::endl;
    }
}