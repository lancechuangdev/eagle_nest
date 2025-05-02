#include "main_window.h"
#include "app_paths.h"
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>

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

    m_builder->get_widget("start_train_model_btn", m_start_train_model_btn);
    if (m_start_train_model_btn)
    {
        m_start_train_model_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_start_train_model_clicked));
    }

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

    m_builder->get_widget("close_training_wizard_btn", m_close_training_wizard_btn);
    if (m_close_training_wizard_btn)
    {
        m_close_training_wizard_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_close_training_wizard_clicked));
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

    m_builder->get_widget("training_wizard_img_included_cbox", m_training_wizard_img_included_cbox);

    m_builder->get_widget("training_wizard_img_category_cbox", m_training_wizard_img_category_cbox);

    m_builder->get_widget("training_wizard_image_refresh_btn", m_training_wizard_image_refresh_btn);
    if (m_training_wizard_image_refresh_btn)
    {
        m_training_wizard_image_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_training_wizard_image_refresh_clicked));
    }

    m_builder->get_widget("training_wizard_images_lbox", m_training_wizard_images_lbox);

    m_builder->get_widget("train_model_btn", m_train_model_btn);
    if (m_train_model_btn)
    {
        m_train_model_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_train_model_clicked));
    }

    m_builder->get_widget("train_model_tview", m_train_model_tview);

    m_builder->get_widget("explorer_stack", m_explorer_stack);

    m_builder->get_widget("explorer_dataset_sources_rbtn", m_explorer_dataset_sources_rbtn);
    if (m_explorer_dataset_sources_rbtn)
    {
        m_explorer_dataset_sources_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_explorer_toggled));
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
                load_image_to_explorer(image_path);
                std::cout << "Row activated for image: " << image_path.string() << std::endl;
            }
        }
    });

    m_builder->get_widget("explorer_image_drawing_area", m_explorer_image_drawing_area);
    m_explorer_image_drawing_area->signal_draw().connect(sigc::mem_fun(*this, &MainWindow::on_explorer_image_draw));
}

MainWindow::~MainWindow()
{
}


bool MainWindow::on_key_press_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrl_pressed = true;
    }
    return Gtk::Window::on_key_press_event(key_event);
}

bool MainWindow::on_key_release_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrl_pressed = false;
    }
    return Gtk::Window::on_key_release_event(key_event);
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
        m_content_stack->set_visible_child(m_active_model_page);
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

void MainWindow::on_start_train_model_clicked()
{
    m_active_model_page = "page_training_wizard";
    m_content_stack->set_visible_child(m_active_model_page);
    m_training_stack->set_visible_child("page_select_model");
    m_current_step = 0;
    m_previous_btn->set_sensitive(false);
    m_next_btn->set_sensitive(true);
    update_step_indicator();

    // Create training WIP directory if it doesn't exist
    if (!std::filesystem::exists(AppPaths::WIP_Path))
    {
        std::filesystem::create_directories(AppPaths::WIP_Path);
    }
    else // Clear the directory if it already exists
    {
        for (const auto &entry : std::filesystem::directory_iterator(AppPaths::WIP_Path))
        {
            std::filesystem::remove_all(entry.path());
        }
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

void MainWindow::on_close_training_wizard_clicked()
{
    // Clear the directory if it already exists
    for (const auto &entry : std::filesystem::directory_iterator(AppPaths::WIP_Path))
    {
        std::filesystem::remove_all(entry.path());
    }

    m_active_model_page = "page_model_welcome";
    m_content_stack->set_visible_child(m_active_model_page);
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

    std::ofstream out(AppPaths::WIP_Path / "model.readme");
    out << std::setw(4) << readme_json << std::endl;
}

void MainWindow::on_training_wizard_image_refresh_clicked()
{
    // Disable the button to prevent multiple clicks
    m_training_wizard_image_refresh_btn->set_sensitive(false);

    // Load images in a separate thread
    std::thread([this]() {
        std::vector<ImageInfo> images_from_training_set;
        std::string selected_img_inclusion = m_training_wizard_img_included_cbox->get_active_text();
        std::string selected_img_category = m_training_wizard_img_category_cbox->get_active_text();

        try
        {
            auto dataset_dir = AppPaths::WIP_Dataset_Path;
            auto dest_json = dataset_dir / "dataset.json";

            // Ensure the dataset directory exists
            if (!fs::exists(dataset_dir))
            {
                fs::create_directories(dataset_dir);
            }

            // If dest_json does not exist, copy it from the source
            if (!fs::exists(dest_json))
            {
                auto source_json = AppPaths::Dataset_Path / "dataset.json";
                fs::copy(source_json, dest_json, fs::copy_options::overwrite_existing);
            }

            // Load existing JSON from destination
            std::ifstream ifs(dest_json);

            if (!ifs)
            {
                std::cerr << "Failed to open dataset.json" << std::endl;
            }
            else
            {
                json images_json;
                ifs >> images_json;
                ifs.close();
                
                for (const auto& entry : images_json)
                {
                    int64_t img_id = entry["img_id"];
                    std::string src_img_path = entry["src_img_path"];
                    std::string dest_img_path = entry["dest_img_path"];
                    std::string source_name = entry["source_name"];
                    std::string source_type = entry["source_type"];
                    std::string category = entry["category"];
                    std::string inclusion = entry["inclusion"];

                    // Filter based on inclusion status
                    if (selected_img_inclusion != "All" && selected_img_inclusion != inclusion)
                        continue;

                    // Filter based on category
                    if (selected_img_category != "All" && selected_img_category != category)
                        continue;
                    
                    images_from_training_set.emplace_back(ImageInfo {
                        img_id,
                        src_img_path,
                        dest_img_path,
                        source_name,
                        source_type,
                        category,
                        inclusion
                    });
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error while scanning images: " << e.what() << '\n';
        }

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this, imgs = std::move(images_from_training_set)]() {
            populate_training_wizard_images_listbox(imgs);
            m_training_wizard_image_refresh_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::populate_training_wizard_images_listbox(const std::vector<ImageInfo>& images)
{
    // clear previous rows
    for (auto* child : m_training_wizard_images_lbox->get_children())
    {
        m_training_wizard_images_lbox->remove(*child);
    }

    // add one row per image
    for (const auto& info : images)
    {
        auto filename = info.dest_img_path.filename().string();
        auto hbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);

        // Label for the image
        auto lbl = Gtk::make_managed<Gtk::Label>(filename);
        lbl->set_xalign(0);
        lbl->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        lbl->set_tooltip_text(filename);
        lbl->set_max_width_chars(40); // Tweak this as needed
        lbl->set_single_line_mode(true); // Prevent wrapping

        // Action button
        auto btn = Gtk::make_managed<Gtk::Button>(info.inclusion == "Included" ? "Remove" : "Add");
        btn->set_valign(Gtk::ALIGN_CENTER);
        btn->signal_clicked().connect([this, btn, info]()
        {
            if (btn->get_label() == "Add")
            {
                update_img_inclusion(info.img_id, "Included");
                btn->set_label("Remove");
            }
            else if (btn->get_label() == "Remove")
            {
                update_img_inclusion(info.img_id, "Excluded");
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
        row->set_data("image_path", new fs::path(info.dest_img_path));

        // Add the row to the listbox
        m_training_wizard_images_lbox->append(*row);
    }
    m_training_wizard_images_lbox->show_all_children();
}

void MainWindow::update_img_inclusion(const int64_t img_id, const std::string& inclusion)
{
    // Load existing JSON
    auto dataset_json = AppPaths::WIP_Dataset_Path / "dataset.json";
    std::ifstream ifs(dataset_json);
    if (!ifs)
    {
        std::cerr << "Failed to open dataset.json" << std::endl;
        return;
    }

    json images_json;
    ifs >> images_json;
    ifs.close();

    // Update the inclusion status
    for (auto& entry : images_json)
    {
        if (entry["img_id"] == img_id)
        {
            entry["inclusion"] = inclusion;
            break;
        }
    }

    // Save the updated JSON
    std::ofstream ofs(dataset_json);
    ofs << std::setw(4) << images_json << std::endl;
}

void MainWindow::on_train_model_clicked()
{
    // Disable the button to prevent multiple clicks
    m_train_model_btn->set_sensitive(false);

    // Load images in a separate thread
    std::thread([this]() {
        // prepare_wip_training_dataset();
        run_train_efficient_ad_model_script("hallelujah", "small", 3); // Example parameters
        convert_efficient_ad_model_to_onnx();
        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this]() {
            m_train_model_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::prepare_wip_training_dataset()
{
    try
    {
        auto dataset_path = AppPaths::WIP_Dataset_Path;
        auto normal_dataset_path = dataset_path / "normal";
        auto abnormal_dataset_path = dataset_path / "abnormal";

        if (fs::exists(normal_dataset_path)) {
            // Remove all contents inside the dataset/normal directory
            fs::remove_all(normal_dataset_path);
        }

        if (fs::exists(abnormal_dataset_path)) {
            // Remove all contents inside the dataset/abnormal directory
            fs::remove_all(abnormal_dataset_path);
        }

        // Recreate the dataset directory and its subdirectories
        fs::create_directories(dataset_path / "normal");
        fs::create_directories(dataset_path / "abnormal");

        // Load existing JSON
        // Copy the image from training set to the WIP directory
        auto dataset_json = dataset_path / "dataset.json";
        if (!fs::exists(dataset_json))
        {
            std::cerr << "dataset.json does not exist" << std::endl;
        }
        else
        {
            std::ifstream ifs(dataset_json);
                
            if (!ifs)
            {
                std::cerr << "Failed to open " << dataset_json << std::endl;
            }
            else
            {
                json images_json;
                ifs >> images_json;
                ifs.close();
                
                for (const auto& entry : images_json)
                {
                    std::string dest_img_path = entry["dest_img_path"];
                    std::string img_name = fs::path(dest_img_path).filename();
                    std::string category = entry["category"];
                    std::string inclusion = entry["inclusion"];

                    // Filter based on inclusion status
                    if (inclusion != "Included")
                        continue;

                    category[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(category[0])));
                    auto dataset_category_path = dataset_path / category;

                    fs::copy(dest_img_path, dataset_category_path / img_name, fs::copy_options::overwrite_existing);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error while preparing training images: " << e.what() << '\n';
    }
}

void MainWindow::run_train_efficient_ad_model_script(const std::string& model_name, const std::string& model_size, int max_epochs)
{
    Glib::RefPtr<Gtk::TextBuffer> buffer = m_train_model_tview->get_buffer();
    std::array<char, 256> buffer_line;

    fs::path script_path = AppPaths::WIP_Path / "train_model.py";
    std::string cmd =
        "bash -c 'source ~/anaconda3/etc/profile.d/conda.sh && "
        "conda activate eagle_nest && "
        "python \"" + script_path.string() + "\" " + model_name + " " + model_size + " " + std::to_string(max_epochs) + "'";

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        buffer->insert(buffer->end(), "Failed to start script.\n");
        return;
    }

    std::thread([this, pipe = std::move(pipe), buffer, buffer_line]() mutable {
        while (fgets(buffer_line.data(), buffer_line.size(), pipe.get()) != nullptr) {
            std::string line(buffer_line.data());
            Glib::signal_idle().connect_once([this, buffer, line = std::string(buffer_line.data())]() {
                buffer->insert(buffer->end(), line);
            });
        }
    }).detach();
}

void MainWindow::on_explorer_toggled()
{
    if (m_explorer_dataset_sources_rbtn->get_active())
    {
        m_explorer_stack->set_visible_child("page_dataset_souces");
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
            
            // Refresh the combo boxes
            refresh_dataset_sources_options();
            refresh_image_category_options();

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

    // Load images in a separate thread
    std::thread([this]() {
        std::vector<ImageInfo> images_from_sources;
        // Get selected data source name from combo box
        std::string selected_source_name = m_dataset_sources_cbox->get_active_text();
        std::string selected_img_category = m_image_category_cbox->get_active_text();
        std::vector<DatasetSource> filtered_sources;
        bool is_training_set = false;

        try
        {
            if (selected_source_name == "Training Set")
            {
                is_training_set = true;

                // Load existing JSON
                auto dataset_json = AppPaths::Dataset_Path / "dataset.json";
                std::ifstream ifs(dataset_json);
                if (!ifs)
                {
                    std::cerr << "Failed to open dataset.json" << std::endl;
                }
                else
                {
                    json images_json;
                    ifs >> images_json;
                    ifs.close();
                    
                    for (const auto& entry : images_json)
                    {
                        int64_t img_id = entry["img_id"];
                        fs::path src_img_path = entry["src_img_path"];
                        fs::path dest_img_path = entry["dest_img_path"];
                        std::string source_name = entry["source_name"];
                        std::string source_type = entry["source_type"];
                        std::string category = entry["category"];
                        
                        if (selected_img_category != "All" && selected_img_category != category)
                            continue;
                        
                        images_from_sources.emplace_back(ImageInfo{
                            img_id,
                            src_img_path,
                            dest_img_path,
                            source_name,
                            source_type,
                            category
                        });
                    }
                }
            }
            else
            {
                for (const auto& source : m_dataset_sources)
                {
                    // Skip display-only sources
                    if (source.dispaly_index >= 0)
                        continue;
            
                    if (selected_source_name == "All Connected Sources")
                    {
                        // No filtering, accept all
                        filtered_sources.push_back(source);
                    }
                    else if (selected_source_name == "Local Datasets")
                    {
                        if (source.type == "Local")
                        {
                            filtered_sources.push_back(source);
                        }
                    }
                    else
                    {
                        // Otherwise match by exact name
                        if (source.name == selected_source_name)
                        {
                            filtered_sources.push_back(source);
                        }
                    }
                }

                for (const auto& source : filtered_sources)
                {
                    std::vector<fs::path> dataset_paths;

                    if (selected_img_category == "All" || selected_img_category == "Normal")
                    {
                        dataset_paths.push_back(fs::path(source.connection_info) / "normal");
                    }
                    if (selected_img_category == "All" || selected_img_category == "Abnormal")
                    {
                        dataset_paths.push_back(fs::path(source.connection_info) / "abnormal");
                    }

                    for (const auto& dataset_path : dataset_paths)
                    {
                        if (fs::exists(dataset_path))
                        {
                            for (const auto& dir_entry : fs::recursive_directory_iterator(dataset_path))
                            {
                                if (dir_entry.is_regular_file() && is_image_file(dir_entry.path()))
                                {
                                    int64_t img_id = 0; // Placeholder for image ID
                                    fs::path src_img_path = dir_entry.path();
                                    fs::path dest_img_path = dir_entry.path();
                                    images_from_sources.emplace_back(ImageInfo{
                                        img_id,
                                        src_img_path,
                                        dest_img_path,
                                        source.name,
                                        source.type,
                                        selected_img_category
                                    });
                                }
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error while scanning images: " << e.what() << '\n';
        }

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this, imgs = std::move(images_from_sources), is_training_set]() {
            populate_explorer_images_listbox(imgs, is_training_set);
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

void MainWindow::populate_explorer_images_listbox(const std::vector<ImageInfo>& images, bool is_training_set)
{
    // clear previous rows
    for (auto* child : m_explorer_images_lbox->get_children())
    m_explorer_images_lbox->remove(*child);
    json images_json;

    // Load existing dataset.json for later use
    // If we are in the training set, we don't need to load the dataset.json
    if (!is_training_set)
    {
        auto dataset_json = AppPaths::Dataset_Path / "dataset.json";
        std::ifstream ifs(dataset_json);
        if (!ifs)
        {
            std::cerr << "Failed to open dataset.json" << std::endl;
        }
        else
        {
            ifs >> images_json;
            ifs.close();
        }
    }

    // add one row per image
    for (const auto& info : images)
    {
        auto filename = info.src_img_path.filename().string();
        auto hbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);

        // Label for the image
        auto lbl = Gtk::make_managed<Gtk::Label>(filename);
        lbl->set_xalign(0);
        lbl->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        lbl->set_tooltip_text(filename);
        lbl->set_max_width_chars(40); // Tweak this as needed
        lbl->set_single_line_mode(true); // Prevent wrapping

        // Check if the image is already in the dataset.json
        bool is_in_dataset = false;
        if (is_training_set)
        {
            is_in_dataset = true;
        }
        else
        {
            auto it = std::remove_if(images_json.begin(), images_json.end(),
            [&info](const json& entry) {
                return entry.contains("src_img_path") &&
                    entry["src_img_path"] == info.src_img_path.string();
            });
            if (it != images_json.end())
            {
                is_in_dataset = true;
            }
        }

        // Action button
        auto action_label = is_in_dataset ? "Remove" : "Add";
        auto btn = Gtk::make_managed<Gtk::Button>(action_label);
        btn->set_valign(Gtk::ALIGN_CENTER);
        btn->signal_clicked().connect([this, btn, info]()
        {
            if (btn->get_label() == "Add")
            {
                on_img_add_clicked(info);
                btn->set_label("Remove");
            }
            else if (btn->get_label() == "Remove")
            {
                on_img_remove_clicked(info);
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
        row->set_data("image_path", new fs::path(info.src_img_path));

        // Add the row to the listbox
        m_explorer_images_lbox->append(*row);
    }
    m_explorer_images_lbox->show_all_children();
}

void MainWindow::on_img_add_clicked(const ImageInfo& image_info)
{
    std::cout << "Add button clicked for: " << image_info.src_img_path.string() << std::endl;
    std::string category = m_ctrl_pressed ? "Abnormal" : "Normal";
    auto dest_img_path = add_image_to_dataset(image_info.src_img_path, category);

    // Create a JSON entry
    json image_entry;
    image_entry["img_id"] = generate_img_id();
    image_entry["src_img_path"] = image_info.src_img_path.string();
    image_entry["dest_img_path"] = dest_img_path.string();
    image_entry["source_name"] = image_info.source_name;
    image_entry["source_type"] = image_info.source_type;
    image_entry["category"] = category;
    image_entry["inclusion"] = "Included";
    
    // Path to the JSON file
    auto dataset_json = AppPaths::Dataset_Path / "dataset.json";

    // Load existing JSON array (if file exists)
    json images_json = json::array();
    if (std::ifstream ifs{dataset_json})
    {
        try
        {
            ifs >> images_json;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to parse existing JSON, resetting: " << e.what() << std::endl;
            images_json = json::array(); // fallback
        }
    }

    // Add the new entry
    images_json.push_back(image_entry);

    // Save updated JSON back
    std::ofstream ofs(dataset_json);
    if (ofs)
    {
        ofs << images_json.dump(4); // pretty-print with indent of 4
    }
    else
    {
        std::cerr << "Failed to open " << dataset_json << " for writing" << std::endl;
    }
}

void MainWindow::on_img_remove_clicked(const ImageInfo& image_info)
{
    std::cout << "Remove button clicked for: " << image_info.dest_img_path.string() << std::endl;
    remove_image_from_dataset(image_info.dest_img_path);

    // Load existing JSON
    auto dataset_json = AppPaths::Dataset_Path / "dataset.json";
    std::ifstream ifs(dataset_json);
    if (!ifs)
    {
        std::cerr << "Failed to open dataset.json" << std::endl;
        return;
    }

    json images_json;
    ifs >> images_json;
    ifs.close();

    // Find and erase the entry matching the file path
    auto it = std::remove_if(images_json.begin(), images_json.end(),
        [&image_info](const json& entry) {
            return entry.contains("img_id") &&
                   entry["img_id"] == image_info.img_id;
        });

    if (it != images_json.end())
    {
        images_json.erase(it, images_json.end());

        // Save updated JSON back to file
        std::ofstream ofs(dataset_json);
        ofs << images_json.dump(4); // pretty-print with indent 4
        ofs.close();
    }
    else
    {
        std::cout << "No matching entry found to remove." << std::endl;
    }
}

fs::path MainWindow::add_image_to_dataset(const fs::path& src_path, const std::string& category)
{
    try
    {
        std::string category_lower = category;
        if (!category_lower.empty())
        {
            category_lower[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(category_lower[0])));
        }
        auto dataset_path = AppPaths::Dataset_Path / category_lower;
        if (!fs::exists(dataset_path))
        {
            fs::create_directories(dataset_path);
        }

        auto dest_path = dataset_path / src_path.filename();
        fs::copy(src_path, dest_path, fs::copy_options::overwrite_existing);
        return dest_path;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to add image: " << ex.what() << std::endl;
        throw;
    }
}

void MainWindow::remove_image_from_dataset(const fs::path& img_path)
{
    try
    {
        if (fs::exists(img_path))
        {
            fs::remove(img_path);
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to remove image: " << ex.what() << std::endl;
    }
}

void MainWindow::load_image_to_explorer(const std::filesystem::path& image_path)
{
    try
    {
        m_loaded_explorer_image_pixbuf = Gdk::Pixbuf::create_from_file(image_path.string());
        m_explorer_image_drawing_area->queue_draw(); // force redraw
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "Failed to load image: " << ex.what() << std::endl;
    }
}

bool MainWindow::on_explorer_image_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (m_loaded_explorer_image_pixbuf)
    {
        // Scale the image to fit the drawing area
        auto allocation = m_explorer_image_drawing_area->get_allocation();
        int area_width = allocation.get_width();
        int area_height = allocation.get_height();

        // Calculate scale ratio
        double scale_x = static_cast<double>(area_width) / m_loaded_explorer_image_pixbuf->get_width();
        double scale_y = static_cast<double>(area_height) / m_loaded_explorer_image_pixbuf->get_height();
        double scale = std::min(scale_x, scale_y);

        cr->save();
        cr->scale(scale, scale);
        Gdk::Cairo::set_source_pixbuf(cr, m_loaded_explorer_image_pixbuf, 0, 0);
        cr->paint();
        cr->restore();
    }

    return true;
}

int64_t MainWindow::generate_img_id()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dis(0, 999);

    return milliseconds * 1000 + dis(gen); // Adds 0–999 jitter
}