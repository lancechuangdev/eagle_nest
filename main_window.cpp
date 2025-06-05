#include "main_window.h"
#include "app_paths.h"
#include "expand_collapse_btn.h"
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <openssl/sha.h>
#include <regex>

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

    m_builder->get_widget("explorer_test_images_rbtn", m_explorer_test_images_rbtn);
    if (m_explorer_test_images_rbtn)
    {
        m_explorer_test_images_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_explorer_toggled));
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

    m_builder->get_widget("train_image_category_cbox", m_train_image_category_cbox);

    m_builder->get_widget("train_images_refresh_btn", m_train_images_refresh_btn);
    if (m_train_images_refresh_btn)
    {
        m_train_images_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_train_images_refresh_clicked));
    }

    m_builder->get_widget("explorer_train_images_lbox", m_explorer_train_images_lbox);
    // Handle row selection
    m_explorer_train_images_lbox->signal_row_activated().connect([this](Gtk::ListBoxRow* row) {
        if (row)
        {
            auto path_ptr = static_cast<fs::path*>(row->get_data("image_path"));
            if (path_ptr)
            {
                const fs::path& image_path = *path_ptr;
                load_image_to_drawing_area(image_path.string(), m_explorer_train_img_pixbuf, m_explorer_train_image_drawing_area);
                std::cout << "Row activated for image: " << image_path << std::endl;
            }
        }
    });

    m_builder->get_widget("explorer_train_image_drawing_area", m_explorer_train_image_drawing_area);
    if (m_explorer_train_image_drawing_area)
    {
        m_explorer_train_image_drawing_area->signal_draw().connect(
            [this](const Cairo::RefPtr<Cairo::Context>& cr) {
                return on_image_draw(cr, 
                    m_explorer_train_img_pixbuf, 
                    Glib::RefPtr<Gdk::Pixbuf>(),
                    m_explorer_train_image_drawing_area);
            }
        );
    }

    m_builder->get_widget("toggle_all_on_train_btn", m_toggle_all_on_train_btn);
    if (m_toggle_all_on_train_btn)
    {
        m_toggle_all_on_train_btn->signal_clicked().connect([this]() {
            m_all_selected_on_train = !m_all_selected_on_train;
        
            set_all_checkboxes(m_explorer_train_images_lbox, m_all_selected_on_train);
        
            // Update the button label
            m_toggle_all_on_train_btn->set_label(m_all_selected_on_train ? "Unselect All" : "Select All");
        });
    }

    m_builder->get_widget("add_train_image_btn", m_add_train_image_btn);
    if (m_add_train_image_btn)
    {
        m_add_train_image_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_add_train_images_clicked));
    }

    m_builder->get_widget("remove_train_image_btn", m_remove_train_image_btn);
    if (m_remove_train_image_btn)
    {
        m_remove_train_image_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_remove_train_images_clicked));
    }

    m_builder->get_widget("explorer_training_selected_count_lbl", m_explorer_training_selected_count_lbl);

    m_builder->get_widget("explorer_training_total_count_lbl", m_explorer_training_total_count_lbl);

    m_builder->get_widget("test_split_ratio_sbtn", m_test_split_ratio_sbtn);

    m_builder->get_widget("auto_split_btn", m_auto_split_btn);
    if (m_auto_split_btn)
    {
        m_auto_split_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_auto_split_clicked));
    }

    m_builder->get_widget("dataset_type_cbox", m_dataset_type_cbox);

    m_builder->get_widget("test_image_category_cbox", m_test_image_category_cbox);

    m_builder->get_widget("test_images_refresh_btn", m_test_images_refresh_btn);
    if (m_test_images_refresh_btn)
    {
        m_test_images_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_test_images_refresh_clicked));
    }

    m_builder->get_widget("explorer_test_images_lbox", m_explorer_test_images_lbox);
    // Handle row selection
    m_explorer_test_images_lbox->signal_row_activated().connect([this](Gtk::ListBoxRow* row) {
        if (row)
        {
            auto path_ptr = static_cast<fs::path*>(row->get_data("image_path"));
            if (path_ptr)
            {
                const fs::path& image_path = *path_ptr;
                load_image_to_drawing_area(image_path.string(), m_explorer_test_img_pixbuf, m_explorer_test_image_drawing_area);
                std::cout << "Row activated for image: " << image_path << std::endl;
            }
        }
    });

    m_builder->get_widget("explorer_test_image_drawing_area", m_explorer_test_image_drawing_area);
    if (m_explorer_test_image_drawing_area)
    {
        m_explorer_test_image_drawing_area->signal_draw().connect(
            [this](const Cairo::RefPtr<Cairo::Context>& cr) {
                return on_image_draw(cr, 
                    m_explorer_test_img_pixbuf, 
                    Glib::RefPtr<Gdk::Pixbuf>(),
                    m_explorer_test_image_drawing_area);
            }
        );
    }

    m_builder->get_widget("toggle_all_on_test_btn", m_toggle_all_on_test_btn);
    if (m_toggle_all_on_test_btn)
    {
        m_toggle_all_on_test_btn->signal_clicked().connect([this]() {
            m_all_selected_on_test = !m_all_selected_on_test;
        
            set_all_checkboxes(m_explorer_test_images_lbox, m_all_selected_on_test);
        
            // Update the button label
            m_toggle_all_on_test_btn->set_label(m_all_selected_on_test ? "Unselect All" : "Select All");
        });
    }

    m_builder->get_widget("add_test_image_btn", m_add_test_image_btn);
    if (m_add_test_image_btn)
    {
        m_add_test_image_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_add_test_images_clicked));
    }

    m_builder->get_widget("remove_test_image_btn", m_remove_test_image_btn);
    if (m_remove_test_image_btn)
    {
        m_remove_test_image_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_remove_test_images_clicked));
    }

    m_builder->get_widget("explorer_test_selected_count_lbl", m_explorer_test_selected_count_lbl);

    m_builder->get_widget("explorer_test_total_count_lbl", m_explorer_test_total_count_lbl);

    m_builder->get_widget("content_stack", m_content_stack);

    m_builder->get_widget("start_train_model_btn", m_start_train_model_btn);
    if (m_start_train_model_btn)
    {
        m_start_train_model_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_start_train_model_clicked));
    }

    m_builder->get_widget("model_performance_evaluation_btn", m_model_performance_evaluation_btn);
    if (m_model_performance_evaluation_btn)
    {
        m_model_performance_evaluation_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_model_performance_evaluation_clicked));
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

    m_builder->get_widget("training_step4_lbl", m_training_step4_lbl);
    if (m_training_step4_lbl)
    {
        m_training_step_labels.push_back(m_training_step4_lbl);
    }

    m_builder->get_widget("training_step5_lbl", m_training_step5_lbl);
    if (m_training_step5_lbl)
    {
        m_training_step_labels.push_back(m_training_step5_lbl);
    }

    m_builder->get_widget("model_name_entry", m_model_name_entry);
    if (m_model_name_entry)
    {
        m_model_name_entry->signal_changed().connect([this]() {
            m_model_name = m_model_name_entry->get_text();
        });
    }

    m_builder->get_widget("existing_models_cbox", m_existing_models_cbox);
    if (m_existing_models_cbox)
    {
        m_existing_models_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_existing_model_selection_changed));
    }

    m_builder->get_widget("model_version_cbox", m_model_version_cbox);
    if (m_model_version_cbox)
    {
        m_model_version_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_model_version_selection_changed));
    }

    m_builder->get_widget("model_comment_tview", m_model_comment_tview);
        
    m_builder->get_widget("create_model_rbtn", m_create_model_rbtn);
    m_builder->get_widget("select_model_rbtn", m_select_model_rbtn);
    if (m_create_model_rbtn && m_select_model_rbtn)
    {
        auto update_ui_state = [this]() {
            bool create_mode = m_create_model_rbtn->get_active();

            m_model_name_entry->set_sensitive(create_mode);
            m_existing_models_cbox->set_sensitive(!create_mode);
            m_model_version_cbox->set_sensitive(!create_mode);

            if (create_mode)
            {
                m_model_name = m_model_name_entry->get_text();
                m_model_version = "v1"; // Reset version to v1
            }
            else
            {
                m_model_name = m_existing_models_cbox->get_active_text();

                // Increment the model version
                std::string selected_version = m_model_version_cbox->get_active_text();
                if (!selected_version.empty())
                {
                    int version_number = std::stoi(selected_version.substr(1));
                    m_model_version = std::string("v") + std::to_string(version_number + 1);        
                }
            }
        };

        m_create_model_rbtn->signal_toggled().connect(update_ui_state);
        m_select_model_rbtn->signal_toggled().connect(update_ui_state);

        // Call once to set the initial state correctly
        update_ui_state();
    }

    update_step_indicator();

    m_builder->get_widget("training_wizard_img_included_cbox", m_training_wizard_img_included_cbox);

    m_builder->get_widget("training_wizard_img_category_cbox", m_training_wizard_img_category_cbox);

    m_builder->get_widget("training_wizard_image_refresh_btn", m_training_wizard_image_refresh_btn);
    if (m_training_wizard_image_refresh_btn)
    {
        m_training_wizard_image_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_training_wizard_image_refresh_clicked));
    }

    m_builder->get_widget("wizard_train_image_drawing_area", m_wizard_train_image_drawing_area);
    if (m_wizard_train_image_drawing_area)
    {
        m_wizard_train_image_drawing_area->signal_draw().connect(
            [this](const Cairo::RefPtr<Cairo::Context>& cr) {
                return on_image_draw(cr, 
                    m_wizard_train_img_pixbuf, 
                    Glib::RefPtr<Gdk::Pixbuf>(), 
                    m_wizard_train_image_drawing_area);
            }
        );
    }

    m_builder->get_widget("training_wizard_images_lbox", m_wizard_train_images_lbox);
    // Handle row selection
    m_wizard_train_images_lbox->signal_row_activated().connect([this](Gtk::ListBoxRow* row) {
        if (row)
        {
            auto path_ptr = static_cast<fs::path*>(row->get_data("image_path"));
            if (path_ptr)
            {
                const fs::path& image_path = *path_ptr;
                load_image_to_drawing_area(image_path.string(), m_wizard_train_img_pixbuf, m_wizard_train_image_drawing_area);
                std::cout << "Row activated for image: " << image_path << std::endl;
            }
        }
    });

    m_builder->get_widget("toggle_all_on_wizard_btn", m_toggle_all_on_wizard_btn);
    if (m_toggle_all_on_wizard_btn)
    {
        m_toggle_all_on_wizard_btn->signal_clicked().connect([this]() {
            m_all_selected_on_wizard = !m_all_selected_on_wizard;
        
            set_all_checkboxes(m_wizard_train_images_lbox, m_all_selected_on_wizard);
        
            // Update the button label
            m_toggle_all_on_wizard_btn->set_label(m_all_selected_on_wizard ? "Unselect All" : "Select All");
        });
    }

    m_builder->get_widget("include_train_images_btn", m_include_train_images_btn);
    if (m_include_train_images_btn)
    {
        m_include_train_images_btn->signal_clicked().connect([this]() {
            update_selected_images_inclusion("Included");
            on_training_wizard_image_refresh_clicked();
        });
    }

    m_builder->get_widget("exclude_train_images_btn", m_exclude_train_images_btn);
    if (m_exclude_train_images_btn)
    {
        m_exclude_train_images_btn->signal_clicked().connect([this]() {
            update_selected_images_inclusion("Excluded");
            on_training_wizard_image_refresh_clicked();
        });
    }

    m_builder->get_widget("wizard_training_selected_count_lbl", m_wizard_training_selected_count_lbl);

    m_builder->get_widget("wizard_training_included_count_lbl", m_wizard_training_included_count_lbl);

    m_builder->get_widget("wizard_training_total_count_lbl", m_wizard_training_total_count_lbl);

    m_builder->get_widget("model_size_cbox", m_model_size_cbox);

    m_builder->get_widget("max_epochs_sbtn", m_max_epochs_sbtn);

    m_builder->get_widget("train_model_btn", m_train_model_btn);
    if (m_train_model_btn)
    {
        m_train_model_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_train_model_clicked));
    }

    m_builder->get_widget("train_model_tview", m_train_model_tview);

    m_builder->get_widget("model_under_test_lbl", m_model_under_test_lbl);

    m_builder->get_widget("model_version_under_test_lbl", m_model_version_under_test_lbl);

    m_builder->get_widget("test_model_btn", m_test_model_btn);
    if (m_test_model_btn)
    {
        m_test_model_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_test_model_clicked));
    }

    m_builder->get_widget("area_under_roc_lbl", m_area_under_roc_lbl);

    m_builder->get_widget("f1_score_lbl", m_f1_score_lbl);

    m_builder->get_widget("wizard_test_images_lbox", m_wizard_test_images_lbox);
    // Handle row selection
    m_wizard_test_images_lbox->signal_row_activated().connect([this](Gtk::ListBoxRow* row) {
        if (row)
        {
            auto path_ptr = static_cast<fs::path*>(row->get_data("image_path"));
            if (path_ptr)
            {
                const fs::path& image_path = *path_ptr;
                load_image_and_heatmap_to_drawing_area(
                    image_path.string(),
                    m_image_to_heatmap_map[image_path.string()].first,
                    m_wizard_test_img_pixbuf,
                    m_wizard_test_heatmap_pixbuf,
                    m_wizard_test_image_drawing_area
                );
                std::cout << "Row activated for image: " << image_path << std::endl;
            }
        }
    });

    m_builder->get_widget("wizard_test_image_drawing_area", m_wizard_test_image_drawing_area);
    if (m_wizard_test_image_drawing_area)
    {
        m_wizard_test_image_drawing_area->signal_draw().connect(
            [this](const Cairo::RefPtr<Cairo::Context>& cr) {
                return on_image_draw(cr, 
                    m_wizard_test_img_pixbuf, 
                    m_wizard_show_heatmap ? m_wizard_test_heatmap_pixbuf : Glib::RefPtr<Gdk::Pixbuf>(), 
                    m_wizard_test_image_drawing_area,
                    0.5f);
            }
        );
    }

    m_builder->get_widget("wizard_show_anomaly_heatmap_switch", m_wizard_show_anomaly_heatmap_switch);
    if (m_wizard_show_anomaly_heatmap_switch)
    {
        m_wizard_show_anomaly_heatmap_switch->property_active().signal_changed().connect([this]() {
            m_wizard_show_heatmap = m_wizard_show_anomaly_heatmap_switch->get_active();
            m_wizard_test_image_drawing_area->queue_draw();
        });
    }

    m_builder->get_widget("wizard_anomaly_score_dist_img_widget", m_wizard_anomaly_score_dist_img_widget);

    m_builder->get_widget("model_name_to_save_lbl", m_model_name_to_save_lbl);

    m_builder->get_widget("model_version_to_save_lbl", m_model_version_to_save_lbl);

    m_builder->get_widget("model_size_to_save_lbl", m_model_size_to_save_lbl);

    m_builder->get_widget("model_comment_to_save_tview", m_model_comment_to_save_tview);

    m_builder->get_widget("save_model_btn", m_save_model_btn);
    if (m_save_model_btn)
    {
        m_save_model_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_save_model_clicked));
    }

    m_builder->get_widget("model1_existing_models_cbox", m_model1_existing_models_cbox);
    if (m_model1_existing_models_cbox)
    {
        m_model1_existing_models_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_model1_existing_models_selection_changed));
    }

    m_builder->get_widget("model1_version_cbox", m_model1_version_cbox);
    if (m_model1_version_cbox)
    {
        m_model1_version_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_model1_version_selection_changed));
    }

    m_builder->get_widget("model1_comment_tview", m_model1_comment_tview);

    m_builder->get_widget("model1_f1_score_lbl", m_model1_f1_score_lbl);

    m_builder->get_widget("model1_area_under_roc_lbl", m_model1_area_under_roc_lbl);

    m_builder->get_widget("model1_anomaly_score_dist_img_widget", m_model1_anomaly_score_dist_img_widget);

    m_builder->get_widget("model1_anomaly_score_dist_zoom_in_btn", m_model1_anomaly_score_dist_zoom_in_btn);
    if (m_model1_anomaly_score_dist_zoom_in_btn)
    {
        m_model1_anomaly_score_dist_zoom_in_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_model1_anomaly_score_dist_zoom_in_clicked));
    }

    m_builder->get_widget("model1_anomaly_score_dist_zoom_out_btn", m_model1_anomaly_score_dist_zoom_out_btn);
    if (m_model1_anomaly_score_dist_zoom_out_btn)
    {
        m_model1_anomaly_score_dist_zoom_out_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_model1_anomaly_score_dist_zoom_out_clicked));
    }

    m_builder->get_widget("model1_anomaly_score_dist_img_ebox", m_model1_anomaly_score_dist_img_ebox);
    if (m_model1_anomaly_score_dist_img_ebox)
    {
        m_model1_anomaly_score_dist_img_ebox->add_events(Gdk::SCROLL_MASK);
        m_model1_anomaly_score_dist_img_ebox->signal_scroll_event().connect(
            sigc::mem_fun(*this, &MainWindow::on_model1_anomaly_score_dist_image_scroll), false);
    }

    m_builder->get_widget("model1_test_image_drawing_area", m_model1_test_image_drawing_area);
    if (m_model1_test_image_drawing_area)
    {
        m_model1_test_image_drawing_area->signal_draw().connect(
            [this](const Cairo::RefPtr<Cairo::Context>& cr) {
                return on_image_draw(cr, 
                    m_model1_test_img_pixbuf, 
                    m_eval_show_heatmap ? m_model1_test_heatmap_pixbuf : Glib::RefPtr<Gdk::Pixbuf>(), 
                    m_model1_test_image_drawing_area,
                    0.5f);
            }
        );
    }

    m_builder->get_widget("model1_test_img_anomaly_score_lbl", m_model1_test_img_anomaly_score_lbl);

    m_builder->get_widget("model2_existing_models_cbox", m_model2_existing_models_cbox);
    if (m_model2_existing_models_cbox)
    {
        m_model2_existing_models_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_model2_existing_models_selection_changed));
    }

    m_builder->get_widget("model2_version_cbox", m_model2_version_cbox);
    if (m_model2_version_cbox)
    {
        m_model2_version_cbox->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_model2_version_selection_changed));
    }

    m_builder->get_widget("model2_comment_tview", m_model2_comment_tview);

    m_builder->get_widget("model2_f1_score_lbl", m_model2_f1_score_lbl);

    m_builder->get_widget("model2_area_under_roc_lbl", m_model2_area_under_roc_lbl);

    m_builder->get_widget("model2_anomaly_score_dist_img_widget", m_model2_anomaly_score_dist_img_widget);

    m_builder->get_widget("model2_anomaly_score_dist_img_ebox", m_model2_anomaly_score_dist_img_ebox);
    if (m_model2_anomaly_score_dist_img_ebox)
    {
        m_model2_anomaly_score_dist_img_ebox->add_events(Gdk::SCROLL_MASK);
        m_model2_anomaly_score_dist_img_ebox->signal_scroll_event().connect(
            sigc::mem_fun(*this, &MainWindow::on_model2_anomaly_score_dist_image_scroll), false);
    }

    m_builder->get_widget("model2_test_image_drawing_area", m_model2_test_image_drawing_area);
    if (m_model2_test_image_drawing_area)
    {
        m_model2_test_image_drawing_area->signal_draw().connect(
            [this](const Cairo::RefPtr<Cairo::Context>& cr) {
                return on_image_draw(cr, 
                    m_model2_test_img_pixbuf, 
                    m_eval_show_heatmap ? m_model2_test_heatmap_pixbuf : Glib::RefPtr<Gdk::Pixbuf>(), 
                    m_model2_test_image_drawing_area,
                    0.5f);
            }
        );
    }

    m_builder->get_widget("model2_test_img_anomaly_score_lbl", m_model2_test_img_anomaly_score_lbl);

    m_builder->get_widget("eval_model_btn", m_eval_model_btn);
    if (m_eval_model_btn)
    {
        m_eval_model_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_eval_model_clicked));
    }

    m_builder->get_widget("eval_back_btn", m_eval_back_btn);
    if (m_eval_back_btn)
    {
        m_eval_back_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_eval_back_clicked));
    }
    
    m_builder->get_widget("eval_next_btn", m_eval_next_btn);
    if (m_eval_next_btn)
    {
        m_eval_next_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_eval_next_clicked));
    }

    m_builder->get_widget("eval_expand_collapse_btn", m_eval_placeholder_btn);
    if (m_eval_placeholder_btn)
    {
        if (auto* box = dynamic_cast<Gtk::Box*>(m_eval_placeholder_btn->get_parent())) {        
            // Remove the placeholder
            box->remove(*m_eval_placeholder_btn);
        
            // Create the custom button
            auto expand_collapse_btn = Gtk::make_managed<ExpandCollapseButton>();
        
            // Insert it in the same position
            box->pack_end(*expand_collapse_btn, Gtk::PACK_SHRINK);
            expand_collapse_btn->show_all();

            // Connect the signal
            expand_collapse_btn->signal_toggled.connect([this](bool expanded) {
                if (expanded) {
                    m_eval_test_images_lbox->show();
                } else {
                    m_eval_test_images_lbox->hide();
                }
                m_eval_test_images_list_revealer->set_reveal_child(expanded);
            });
        }
    }

    m_builder->get_widget("eval_test_images_list_revealer", m_eval_test_images_list_revealer);

    m_builder->get_widget("eval_test_images_lbox", m_eval_test_images_lbox);
    m_eval_test_images_lbox->signal_row_activated().connect([this](Gtk::ListBoxRow* row) {
        activate_eval_testing_images_row(row);
    });

    m_builder->get_widget("eval_show_anomaly_heatmap_switch", m_eval_show_anomaly_heatmap_switch);
    if (m_eval_show_anomaly_heatmap_switch)
    {
        m_eval_show_anomaly_heatmap_switch->property_active().signal_changed().connect([this]() {
            m_eval_show_heatmap = m_eval_show_anomaly_heatmap_switch->get_active();
            m_model1_test_image_drawing_area->queue_draw();
            m_model2_test_image_drawing_area->queue_draw();
        });
    }
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
    else if (m_explorer_test_images_rbtn->get_active())
    {
        m_explorer_stack->set_visible_child("page_test_images");
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

void MainWindow::on_train_images_refresh_clicked()
{
    // Disable the button to prevent multiple clicks
    m_train_images_refresh_btn->set_sensitive(false);

    // Load images in a separate thread
    std::thread([this]() {
        // Get selected data source name from combo box
        std::string selected_source_name = m_dataset_sources_cbox->get_active_text();
        std::string selected_img_category = m_train_image_category_cbox->get_active_text();
        int training_images_count = 0;

        // Clear previous images
        m_images_from_datasources.clear();

        // Load existing JSON
        json images_json;
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

        try
        {
            for (const auto& entry : images_json)
            {
                std::string inclusion = entry["inclusion"];
                std::string dataset_type = entry["dataset_type"];

                if (dataset_type != "train")
                    continue;
                
                if (inclusion != "Included")
                    continue;
                
                training_images_count++;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to retrieve the training images count: " << e.what() << '\n';
        }

        try
        {
            if (selected_source_name == "Training Set")
            {
                for (const auto& entry : images_json)
                {
                    std::string img_id = entry["img_id"];
                    fs::path src_img_path = entry["src_img_path"];
                    fs::path dest_img_path = entry["dest_img_path"];
                    std::string source_name = entry["source_name"];
                    std::string source_type = entry["source_type"];
                    std::string category = entry["category"];
                    std::string inclusion = entry["inclusion"];
                    std::string dataset_type = entry["dataset_type"];

                    if (dataset_type != "train")
                        continue;
                    
                    if (selected_img_category != category)
                        continue;
                    
                    m_images_from_datasources.emplace_back(ImageInfo{
                        img_id,
                        src_img_path,
                        dest_img_path,
                        source_name,
                        source_type,
                        category,
                        inclusion,
                        dataset_type
                    });
                }
            }
            else
            {
                std::vector<DatasetSource> filtered_sources;

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

                    if (selected_img_category == "normal")
                    {
                        dataset_paths.push_back(fs::path(source.connection_info) / "normal");
                    }
                    else if (selected_img_category == "abnormal")
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
                                    std::string src_img_path_str = dir_entry.path().string();

                                    // Check if the image is already in the dataset
                                    auto it = std::find_if(images_json.begin(), images_json.end(),
                                        [src_img_path_str](const json& entry) {
                                            return entry["src_img_path"] == src_img_path_str;
                                        });
                                    
                                    if (it != images_json.end())
                                    {
                                        auto& image_entry = *it;
                                        m_images_from_datasources.emplace_back(ImageInfo{
                                            image_entry["img_id"],
                                            image_entry["src_img_path"],
                                            image_entry["dest_img_path"],
                                            image_entry["source_name"],
                                            image_entry["source_type"],
                                            image_entry["category"],
                                            image_entry["inclusion"],
                                            image_entry["dataset_type"]
                                        });
                                    }
                                    else
                                    {
                                        std::string img_id = generate_sha256(src_img_path_str);
                                        fs::path src_img_path = dir_entry.path();
                                        fs::path dest_img_path = fs::path(); // unknown destination path
                                        m_images_from_datasources.emplace_back(ImageInfo{
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
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error while scanning images: " << e.what() << '\n';
        }

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this, training_images_count]() {
            populate_explorer_train_images_listbox();
            m_toggle_all_on_train_btn->set_label("Select All");
            m_selected_images_on_train_listbox.clear();
            m_explorer_training_selected_count_lbl->set_text("0");
            m_explorer_training_total_count_lbl->set_text(std::to_string(training_images_count));
            m_train_images_refresh_btn->set_sensitive(true);
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

void MainWindow::populate_explorer_train_images_listbox()
{
    // clear previous rows
    for (auto* child : m_explorer_train_images_lbox->get_children()) {
        m_explorer_train_images_lbox->remove(*child);
    }

    json images_json;

    // add one row per image
    for (const ImageInfo &info : m_images_from_datasources)
    {
        auto filename = info.src_img_path.filename().string();

        // Outer vertical box for header and details
        auto vbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);

        // Top row: checkbox + filename label + "Info" button
        auto hbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);

        // Checkbox
        auto checkbox = Gtk::make_managed<Gtk::CheckButton>();
        checkbox->set_halign(Gtk::Align::ALIGN_CENTER);
        hbox->pack_start(*checkbox, Gtk::PACK_SHRINK);
        checkbox->signal_toggled().connect([this, checkbox, info]() {
            bool is_checked = checkbox->get_active();
            if (is_checked)
                m_selected_images_on_train_listbox[checkbox] = info.img_id;
            else
                m_selected_images_on_train_listbox.erase(checkbox);

            // Update the image count label based on selection
            m_explorer_training_selected_count_lbl->set_text(std::to_string(m_selected_images_on_train_listbox.size()));
        });

        auto lbl = Gtk::make_managed<Gtk::Label>(filename);
        lbl->set_xalign(0);
        lbl->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        lbl->set_tooltip_text(filename);
        lbl->set_max_width_chars(40);
        lbl->set_single_line_mode(true);
        hbox->pack_start(*lbl, Gtk::PACK_EXPAND_WIDGET);

        auto toggle_btn = Gtk::make_managed<ExpandCollapseButton>();
        hbox->pack_start(*toggle_btn, Gtk::PACK_SHRINK);

        // Detail content
        auto details_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);
        auto dataset_type_label = Gtk::make_managed<Gtk::Label>("Dataset Type" + std::string(": ") + info.dataset_type);
        dataset_type_label->set_xalign(0);
        auto category_label = Gtk::make_managed<Gtk::Label>("Category" + std::string(": ") + info.category);
        category_label->set_xalign(0);
        auto source_name_label = Gtk::make_managed<Gtk::Label>("Data Source Name" + std::string(": ") + info.source_name);
        source_name_label->set_xalign(0);
        auto source_type_label = Gtk::make_managed<Gtk::Label>("Data Source Type" + std::string(": ") + info.source_type);
        source_type_label->set_xalign(0);
        auto img_path_label = Gtk::make_managed<Gtk::Label>("Image Path" + std::string(": ") + info.dest_img_path.string());
        img_path_label->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        img_path_label->set_max_width_chars(40); // Limit display width
        img_path_label->set_tooltip_text(info.dest_img_path.string());
        img_path_label->set_xalign(0); // Align left
        details_box->pack_start(*dataset_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*category_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_name_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*img_path_label, Gtk::PACK_SHRINK);
        details_box->set_margin_start(5);
        details_box->set_margin_end(5);
        details_box->set_margin_top(5);
        details_box->set_margin_bottom(5);

        // Wrap detail box in a Revealer
        auto revealer = Gtk::make_managed<Gtk::Revealer>();
        revealer->set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
        revealer->set_transition_duration(200);
        revealer->add(*details_box);
        revealer->set_reveal_child(false);  // initially hidden

        // Toggle the Revealer when the button is clicked
        toggle_btn->signal_toggled.connect([revealer](bool expanded) {
            revealer->set_reveal_child(expanded);
        });

        // Pack into vertical container
        vbox->pack_start(*hbox, Gtk::PACK_SHRINK);
        vbox->pack_start(*revealer, Gtk::PACK_SHRINK);

        // Create row and add to listbox
        auto row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->add(*vbox);

        // Store the path as custom data
        row->set_data("image_path", new fs::path(info.src_img_path));

        // Add the row to the listbox
        m_explorer_train_images_lbox->append(*row);
    }
    m_explorer_train_images_lbox->show_all_children();
}

void MainWindow::add_selected_images_to_train(std::map<Gtk::CheckButton*, std::string> selected_images)
{
    std::string category = m_ctrl_pressed ? "abnormal" : "normal";

    // Load dataset.json
    auto dataset_json = AppPaths::Dataset_Path / "dataset.json";
    json images_json = json::array();
    std::ifstream ifs(dataset_json);
    if (!ifs)
    {
        std::cerr << "Failed to open dataset.json, or it doesn't exist" << std::endl;
    }
    else if (std::ifstream ifs{dataset_json})
    {
        try
        {
            ifs >> images_json;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to parse existing JSON, resetting: " << e.what() << std::endl;
        }
        ifs.close();
    }

    for (const auto& [checkbox, img_id] : selected_images)
    {
        std::cout << "Selected image ID: " << img_id << std::endl;

        // Check if the image is already in the dataset
        auto it = std::find_if(images_json.begin(), images_json.end(),
            [img_id](const json& entry) {
                return entry["img_id"] == img_id;
            });
        if (it != images_json.end())
        {
            std::cerr << "Image ID " << img_id << " already exists in the dataset." << std::endl;
            continue; // Skip this image
        }

        // Find the corresponding image info from the data source
        auto it2 = std::find_if(m_images_from_datasources.begin(), m_images_from_datasources.end(),
            [img_id](const ImageInfo& info) {
                return info.img_id == img_id;
            });
    
        if (it2 != m_images_from_datasources.end()) {
            auto& image_info = *it2;
            auto dest_img_path = copy_image_to_dataset(image_info.src_img_path, "train", category);

            // Create a JSON entry
            json image_entry;
            image_entry["img_id"] = image_info.img_id;
            image_entry["src_img_path"] = image_info.src_img_path.string();
            image_entry["dest_img_path"] = dest_img_path.string();
            image_entry["source_name"] = image_info.source_name;
            image_entry["source_type"] = image_info.source_type;
            image_entry["category"] = category;
            image_entry["inclusion"] = "Included";
            image_entry["dataset_type"] = "train";

            // Add the new entry
            images_json.push_back(image_entry);
        }
        else
        {
            std::cerr << "Image ID " << img_id << " not found in the data source." << std::endl;
        }
    }

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

    // Clear the selected images
    selected_images.clear();
}

void MainWindow::remove_selected_images_from_train(std::map<Gtk::CheckButton*, std::string> selected_images)
{
    // Load dataset.json
    auto dataset_json = AppPaths::Dataset_Path / "dataset.json";
    std::ifstream ifs(dataset_json);
    if (!ifs)
    {
        std::cerr << "Failed to open dataset.json" << std::endl;
        return;
    }
    
    json images_json;
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
    ifs.close();

    for (const auto& [checkbox, img_id] : selected_images)
    {
        std::cout << "Selected image ID: " << img_id << std::endl;

        // Check if the image is already in the dataset
        auto it = std::find_if(images_json.begin(), images_json.end(),
            [img_id](const json& entry) {
                return entry["img_id"] == img_id;
            });
        if (it == images_json.end())
        {
            std::cerr << "Image ID " << img_id << " already exists in the dataset." << std::endl;
            continue; // Skip this image
        }

        // Find and erase the corresponding image info
        std::string dest_img_path = (*it)["dest_img_path"];
        remove_image_from_dataset(dest_img_path);
        images_json.erase(it, images_json.end());
    }

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

    // Clear the selected images
    selected_images.clear();
}


void MainWindow::on_add_train_images_clicked()
{
    add_selected_images_to_train(m_selected_images_on_train_listbox);

    // Refresh the train images listbox
    on_train_images_refresh_clicked();
}

void MainWindow::on_remove_train_images_clicked()
{
    remove_selected_images_from_train(m_selected_images_on_train_listbox);

    // Refresh the train images listbox
    on_train_images_refresh_clicked();
}

fs::path MainWindow::copy_image_to_dataset(const fs::path& src_path, const std::string& dataset_type, const std::string& category)
{
    try
    {
        auto dataset_path = AppPaths::Dataset_Path / dataset_type / category;
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

void MainWindow::load_image_to_drawing_area(const std::string& image_path, Glib::RefPtr<Gdk::Pixbuf>& pixbuf, Gtk::DrawingArea* target_drawing_area)
{
    try
    {
        pixbuf = Gdk::Pixbuf::create_from_file(image_path);
        if (target_drawing_area)
        {
            target_drawing_area->queue_draw(); // force redraw
        }
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "Failed to load image: " << ex.what() << std::endl;
    }
}

bool MainWindow::on_image_draw(const Cairo::RefPtr<Cairo::Context>& cr,
    Glib::RefPtr<Gdk::Pixbuf> base_pixbuf,
    Glib::RefPtr<Gdk::Pixbuf> overlay_pixbuf,
    Gtk::DrawingArea* area,
    double overlay_alpha)
{
    if (!base_pixbuf || !area)
    return true;

    auto allocation = area->get_allocation();
    const int area_w = allocation.get_width();
    const int area_h = allocation.get_height();

    // Use base image size
    const int img_w = base_pixbuf->get_width();
    const int img_h = base_pixbuf->get_height();

    double scale_x = static_cast<double>(area_w) / img_w;
    double scale_y = static_cast<double>(area_h) / img_h;
    double scale = std::min(scale_x, scale_y);

    // Centering offset
    double dx = (area_w  - img_w * scale) * 0.5;
    double dy = (area_h - img_h * scale) * 0.5;

    auto draw_scaled_pixbuf = [&](Glib::RefPtr<Gdk::Pixbuf> pixbuf, double alpha) {
        if (!pixbuf) return;

        cr->save();
        cr->translate(dx, dy);
        cr->scale(scale, scale);

        Gdk::Cairo::set_source_pixbuf(cr, pixbuf, 0, 0);
        if (alpha < 1.0) {
            cr->paint_with_alpha(alpha);
        } else {
            cr->paint();
        }

        cr->restore();
    };

    // Draw base image
    draw_scaled_pixbuf(base_pixbuf, 1.0);

    // Draw overlay if available
    if (overlay_pixbuf) {
        draw_scaled_pixbuf(overlay_pixbuf, overlay_alpha);
    }

    return true;
}


void MainWindow::load_image_and_heatmap_to_drawing_area(
    const std::string& image_path,
    const std::string& heatmap_path,
    Glib::RefPtr<Gdk::Pixbuf>& image_pixbuf,
    Glib::RefPtr<Gdk::Pixbuf>& heatmap_pixbuf,
    Gtk::DrawingArea* target_drawing_area)
{
    try {
        image_pixbuf = Gdk::Pixbuf::create_from_file(image_path);

        if (!heatmap_path.empty() && std::filesystem::exists(heatmap_path)) {
            heatmap_pixbuf = Gdk::Pixbuf::create_from_file(heatmap_path)
                ->scale_simple(image_pixbuf->get_width(), image_pixbuf->get_height(), Gdk::INTERP_BILINEAR);
        } else {
            heatmap_pixbuf.reset();  // Clear
        }

        if (target_drawing_area) {
            target_drawing_area->queue_draw();
        }
    } catch (const Glib::Error& ex) {
        std::cerr << "Error loading image or heatmap: " << ex.what() << std::endl;
    }
}

std::string MainWindow::generate_sha256(const std::string& input)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    std::ostringstream result;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        result << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return result.str();
}

void MainWindow::on_auto_split_clicked()
{
    // Disable the button to prevent multiple clicks
    m_auto_split_btn->set_sensitive(false);

    // Perform auto-split in a separate thread
    std::thread([this]() {
        double split_ratio = m_test_split_ratio_sbtn->get_value() / 100.0;
        auto dataset_json = AppPaths::Dataset_Path / "dataset.json";
        std::ifstream ifs(dataset_json);
        if (!ifs)
        {
            std::cerr << "Failed to open dataset.json" << std::endl;
            return;
        }

        // Load existing JSON
        json images_json;
        ifs >> images_json;
        ifs.close();
        if (images_json.empty())
        {
            std::cerr << "No images found in dataset.json" << std::endl;
            return;
        }

        // Mark all as Training
        for (auto& entry : images_json)
        {
            entry["dataset_type"] = "train";
        }

        std::vector<std::reference_wrapper<json>> normal_images;
        std::vector<std::reference_wrapper<json>> abnormal_images;

        for (auto& entry : images_json)
        {
            if (entry["category"] == "normal")
                normal_images.push_back(entry);
            else
                abnormal_images.push_back(entry);
        }

        // Shuffle both groups
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(normal_images.begin(), normal_images.end(), g);
        std::shuffle(abnormal_images.begin(), abnormal_images.end(), g);
     
        // Split and Assign "Testing" dataset_type
        auto split = [split_ratio](std::vector<std::reference_wrapper<json>>& group, const std::string& category)
        {
            size_t test_count = static_cast<size_t>(group.size() * split_ratio);
            for (size_t i = 0; i < group.size(); ++i)
            {
                if (i < test_count)
                {
                    std::string from = group[i].get()["dest_img_path"];
                    fs::path to = AppPaths::Dataset_Path / "test" / category;
                    if (!fs::exists(to))
                    {
                        fs::create_directories(to);
                    }
                    std::string img_name = fs::path(from).filename();
                    to /= img_name;
                    fs::rename(from, to);
                    group[i].get()["dataset_type"] = "test";
                    group[i].get()["dest_img_path"] = to.string();
                }
            }
        };
        split(normal_images, "normal");
        split(abnormal_images, "abnormal");

        // Save back
        std::ofstream ofs(dataset_json);
        if (ofs)
        {
            ofs << images_json.dump(4);
        }
        else
        {
            std::cerr << "Failed to open " << dataset_json << " for writing" << std::endl;
        }
        
        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this]() {
            m_auto_split_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::on_test_images_refresh_clicked()
{
    // Disable the button to prevent multiple clicks
    m_test_images_refresh_btn->set_sensitive(false);

    std::string selected_dataset_type = m_dataset_type_cbox->get_active_text();
    std::string selected_img_category = m_test_image_category_cbox->get_active_text();
    
    // Load images in a separate thread
    std::thread([this, selected_dataset_type = std::move(selected_dataset_type),
        selected_img_category = std::move(selected_img_category)]()
    {
        std::vector<ImageInfo> filtered_images;
        int test_images_count = 0;
        json images_json;

        // Load existing JSON
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

        for (const auto& entry : images_json)
        {
            const std::string& img_id = entry["img_id"];
            const fs::path src_img_path = entry["src_img_path"];
            const fs::path dest_img_path = entry["dest_img_path"];
            const std::string& source_name  = entry["source_name"];
            const std::string& source_type  = entry["source_type"];
            const std::string& category = entry["category"];
            const std::string& inclusion = entry["inclusion"];
            const std::string& dataset_type = entry["dataset_type"];

            // Count test images that are included
            if (dataset_type == "test" && inclusion == "Included")
            {
                ++test_images_count;
            }

            // Filter images by selected dataset and category
            if (dataset_type == selected_dataset_type && category == selected_img_category)
            {
                filtered_images.emplace_back(ImageInfo{
                    img_id,
                    src_img_path,
                    dest_img_path,
                    source_name,
                    source_type,
                    category,
                    inclusion,
                    dataset_type
                });
            }
        }

        Glib::signal_idle().connect_once([this, imgs = std::move(filtered_images), test_images_count]() {
            populate_explorer_test_images_listbox(imgs);
            m_toggle_all_on_test_btn->set_label("Select All");
            m_selected_images_on_test_listbox.clear();
            m_explorer_test_selected_count_lbl->set_text("0");
            m_explorer_test_total_count_lbl->set_text(std::to_string(test_images_count));
            m_test_images_refresh_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::populate_explorer_test_images_listbox(const std::vector<ImageInfo>& images)
{
    // Clear previous rows
    for (auto* child : m_explorer_test_images_lbox->get_children()) {
        m_explorer_test_images_lbox->remove(*child);
    }

    for (const auto& info : images)
    {
        auto filename = info.src_img_path.filename().string();

        // Outer vertical box for header and details
        auto vbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);

        // Top row: checkbox + filename label + "Info" button
        auto hbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);

        // Checkbox
        auto checkbox = Gtk::make_managed<Gtk::CheckButton>();
        checkbox->set_halign(Gtk::Align::ALIGN_CENTER);
        hbox->pack_start(*checkbox, Gtk::PACK_SHRINK);
        checkbox->signal_toggled().connect([this, checkbox, info]() {
            bool is_checked = checkbox->get_active();
            if (is_checked)
                m_selected_images_on_test_listbox[checkbox] = info.img_id;
            else
                m_selected_images_on_test_listbox.erase(checkbox);

            // Update the image count label based on selection
            m_explorer_test_selected_count_lbl->set_text(std::to_string(m_selected_images_on_test_listbox.size()));
        });

        auto lbl = Gtk::make_managed<Gtk::Label>(filename);
        lbl->set_xalign(0);
        lbl->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        lbl->set_tooltip_text(filename);
        lbl->set_max_width_chars(40);
        lbl->set_single_line_mode(true);

        hbox->pack_start(*lbl, Gtk::PACK_EXPAND_WIDGET);

        auto toggle_btn = Gtk::make_managed<ExpandCollapseButton>();
        hbox->pack_start(*toggle_btn, Gtk::PACK_SHRINK);

        // Detail content
        auto details_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);
        auto dataset_type_label = Gtk::make_managed<Gtk::Label>("Dataset Type" + std::string(": ") + info.dataset_type);
        dataset_type_label->set_xalign(0);
        auto category_label = Gtk::make_managed<Gtk::Label>("Category" + std::string(": ") + info.category);
        category_label->set_xalign(0);
        auto source_name_label = Gtk::make_managed<Gtk::Label>("Data Source Name" + std::string(": ") + info.source_name);
        source_name_label->set_xalign(0);
        auto source_type_label = Gtk::make_managed<Gtk::Label>("Data Source Type" + std::string(": ") + info.source_type);
        source_type_label->set_xalign(0);
        auto img_path_label = Gtk::make_managed<Gtk::Label>("Image Path" + std::string(": ") + info.dest_img_path.string());
        img_path_label->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        img_path_label->set_max_width_chars(40); // Limit display width
        img_path_label->set_tooltip_text(info.dest_img_path.string());
        img_path_label->set_xalign(0); // Align left
        details_box->pack_start(*dataset_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*category_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_name_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*img_path_label, Gtk::PACK_SHRINK);
        details_box->set_margin_start(5);
        details_box->set_margin_end(5);
        details_box->set_margin_top(5);
        details_box->set_margin_bottom(5);

        // Wrap detail box in a Revealer
        auto revealer = Gtk::make_managed<Gtk::Revealer>();
        revealer->set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
        revealer->set_transition_duration(200);
        revealer->add(*details_box);
        revealer->set_reveal_child(false);  // initially hidden

        // Toggle the Revealer when the button is clicked
        toggle_btn->signal_toggled.connect([revealer](bool expanded) {
            revealer->set_reveal_child(expanded);
        });

        // Pack into vertical container
        vbox->pack_start(*hbox, Gtk::PACK_SHRINK);
        vbox->pack_start(*revealer, Gtk::PACK_SHRINK);

        // Create row and add to listbox
        auto row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->add(*vbox);
        row->set_data("image_path", new fs::path(info.dest_img_path));

        m_explorer_test_images_lbox->append(*row);

        // Show everything
        vbox->show_all();  // includes hbox, but not details_box yet
        row->show();
    }

    m_explorer_test_images_lbox->show_all_children();
}

void MainWindow::set_all_checkboxes(Gtk::ListBox *images_lbox, bool checked)
{
    for (auto* child : images_lbox->get_children()) {
        auto row = dynamic_cast<Gtk::ListBoxRow*>(child);
        if (!row) continue;

        auto vbox = dynamic_cast<Gtk::Box*>(row->get_child());
        if (!vbox) continue;

        auto hbox = dynamic_cast<Gtk::Box*>(vbox->get_children()[0]);
        if (!hbox) continue;

        auto checkbox = dynamic_cast<Gtk::CheckButton*>(hbox->get_children()[0]);
        if (checkbox) checkbox->set_active(checked);  // Will update m_selected_checkboxes too
    }
}

void MainWindow::move_selected_images(std::map<Gtk::CheckButton*, std::string> selected_images, std::string dataset_type)
{
    // Load dataset.json
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

    for (const auto& [checkbox, img_id] : selected_images)
    {
        std::cout << "Selected image ID: " << img_id << std::endl;
        // Find the corresponding image in images_json
        auto it = std::find_if(images_json.begin(), images_json.end(),
            [img_id](const json& entry) {
                return entry.contains("img_id") && entry["img_id"] == img_id;
            });
        if (it != images_json.end())
        {
            // Print the image path
            std::cout << "Image Path: " << (*it)["dest_img_path"] << std::endl;

            // check if the image is already in the test set
            if ((*it)["dataset_type"] == dataset_type)
            {
                std::cout << "Image is already in the test set." << std::endl;
            }
            else
            {
                std::string from = (*it)["dest_img_path"];
                std::string category = (*it)["category"];
                fs::path to = AppPaths::Dataset_Path / dataset_type / category;
                if (!fs::exists(to))
                {
                    fs::create_directories(to);
                }
                std::string img_name = fs::path(from).filename();
                to /= img_name;
                fs::rename(from, to);
                (*it)["dataset_type"] = dataset_type;
                (*it)["dest_img_path"] = to.string();
            }
        }
        else
        {
            std::cout << "Image ID not found in dataset.json" << std::endl;
        }
    }

    // Save back
    std::ofstream ofs(dataset_json);
    if (ofs)
    {
        ofs << images_json.dump(4);
    }
    else
    {
        std::cerr << "Failed to open " << dataset_json << " for writing" << std::endl;
    }

    // Clear the selected images
    selected_images.clear();
}

void MainWindow::on_add_test_images_clicked()
{
    move_selected_images(m_selected_images_on_test_listbox, "test");

    // Refresh the test images listbox
    on_test_images_refresh_clicked();
}

void MainWindow::on_remove_test_images_clicked()
{
    move_selected_images(m_selected_images_on_test_listbox, "train");

    // Refresh the test images listbox
    on_test_images_refresh_clicked();
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

    // Prepare the WIP directory and copy scripts
    prepare_wip_scripts();

    // Load all existing trained models and populate the model name combo box
    auto models = get_existing_models();
    m_existing_models_cbox->remove_all(); // Clear existing items
    for (const auto& model_name : models)
    {
        m_existing_models_cbox->append(model_name);
    }
}

void MainWindow::on_model_performance_evaluation_clicked()
{
    m_active_model_page = "page_model_performance_evaluation";
    m_content_stack->set_visible_child(m_active_model_page);

    // Prepare the WIP directory and copy scripts
    prepare_wip_scripts();

    // Load all existing trained models and populate the model name combo box
    auto models = get_existing_models();
    m_model1_existing_models_cbox->remove_all(); // Clear existing items
    m_model2_existing_models_cbox->remove_all(); // Clear existing items
    for (const auto& model_name : models)
    {
        m_model1_existing_models_cbox->append(model_name);
        m_model2_existing_models_cbox->append(model_name);
    }
}

void MainWindow::prepare_wip_scripts()
{
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

    // Copy all python scripts from the install share directory to the WIP directory
    auto install_share_dir = AppPaths::Install_Share_Dir;
    auto wip_dir = AppPaths::WIP_Path;
    for (const auto &entry : std::filesystem::directory_iterator(install_share_dir))
    {
        if (entry.path().extension() == ".py")
        {
            std::filesystem::copy(entry.path(), wip_dir / entry.path().filename());
        }
    }
}

std::vector<std::string> MainWindow::get_existing_models()
{
    std::vector<std::string> models;
    auto models_path = AppPaths::Models_Path;

    m_existing_models_cbox->remove_all(); // Clear existing items

    if (!fs::exists(models_path) || !fs::is_directory(models_path))
        return models;

    for (const auto& entry : fs::directory_iterator(models_path))
    {
        if (entry.is_directory())
        {
            std::string model_name = entry.path().filename().string();
            models.push_back(model_name);
        }
    }

    return models;
}

void MainWindow::on_existing_model_selection_changed()
{
    std::string selected_model = m_existing_models_cbox->get_active_text();
    if (selected_model.empty())
        return;

    // Update the model name field
    m_model_name = selected_model;

    // Clear the model version combo box
    m_model_version_cbox->remove_all();

    // Populate the model version combo box with available versions
    auto model_path = AppPaths::Models_Path / selected_model;
    if (fs::exists(model_path) && fs::is_directory(model_path))
    {
        for (const auto& entry : fs::directory_iterator(model_path))
        {
            if (entry.is_directory())
            {
                std::string version = entry.path().filename().string();
                m_model_version_cbox->append(version);
            }
        }
    }
}

void MainWindow::on_model_version_selection_changed()
{
    std::string selected_model = m_existing_models_cbox->get_active_text();
    std::string selected_version = m_model_version_cbox->get_active_text();

    if (selected_model.empty() || selected_version.empty())
        return;

    // Increment the model version
    int version_number = std::stoi(selected_version.substr(1));
    m_model_version = std::string("v") + std::to_string(version_number + 1);

    // Load the model readme file and populate the comment text view
    auto model_path = AppPaths::Models_Path / selected_model / selected_version / "model.readme";
    if (fs::exists(model_path))
    {
        std::ifstream ifs(model_path);
        if (ifs)
        {
            json readme_json;
            ifs >> readme_json;
            ifs.close();

            // Populate the model comment text view
            m_model_comment_tview->get_buffer()->set_text(readme_json["comment"]);
        }
    }
    else
    {
        std::cerr << "Model readme file not found." << std::endl;
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

    // Reset the training wizard state
    m_current_step = 0;
    m_model_name = "";
    m_model_version = "v1"; // Reset version to v1
    m_model_size = "";
    m_model_name_entry->set_text("");
    m_existing_models_cbox->remove_all();
    m_model_version_cbox->remove_all();
    m_model_comment_tview->get_buffer()->set_text("");
    for (auto* child : m_wizard_train_images_lbox->get_children())
    {
        m_wizard_train_images_lbox->remove(*child);
    }
    m_wizard_train_img_pixbuf.reset();
    m_selected_images_on_wizard_listbox.clear();
    m_wizard_training_selected_count_lbl->set_text("0");
    m_wizard_training_included_count_lbl->set_text("0");
    m_wizard_training_total_count_lbl->set_text("0");
    m_train_model_tview->get_buffer()->set_text("");
    m_auroc_value = 0.0;
    m_f1_value = 0.0;
    m_f1_score_lbl->set_text("");
    m_area_under_roc_lbl->set_text("");
    for (auto* child : m_wizard_test_images_lbox->get_children())
    {
        m_wizard_test_images_lbox->remove(*child);
    }
    m_wizard_test_img_pixbuf.reset();
    m_wizard_test_heatmap_pixbuf.reset();
    m_wizard_anomaly_score_dist_img_widget->clear();
    m_model_name_to_save_lbl->set_text("");
    m_model_version_to_save_lbl->set_text("");
    m_model_size_to_save_lbl->set_text("");
    m_model_comment_to_save_tview->get_buffer()->set_text("");
    m_previous_btn->set_sensitive(false);
    m_next_btn->set_sensitive(true);

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
        if (m_current_step == 1) // Step 0 (Select Model) to Step 1 (Select Images)
        {
            
        }
        else if (m_current_step == 2) // Step 1 (Select Images) to Step 2 (Training)
        {
        }
        else if (m_current_step == 3) // Step 2 (Training) to Step 3 (Testing)
        {
            // Update the model name and version labels on Testing page
            m_model_under_test_lbl->set_text(m_model_name);
            m_model_version_under_test_lbl->set_text(m_model_version);
        }
        else if (m_current_step == 4) // Step 3 (Testing) to Step 4 (Save Model)
        {
            m_model_name_to_save_lbl->set_text(m_model_name);
            m_model_version_to_save_lbl->set_text(m_model_version);
            m_model_size_to_save_lbl->set_text(m_model_size);
            m_model_comment_to_save_tview->get_buffer()->set_text(m_model_comment_tview->get_buffer()->get_text());
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

void MainWindow::write_model_readme(const std::string& name, const std::string& version, const std::string& size, const int epochs, const std::string& comment, const double auroc_value, const double f1_value)
{
    // Get current datetime in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_now), "%Y-%m-%dT%H:%M:%S");

    nlohmann::json readme_json;
    readme_json["name"] = name;
    readme_json["version"] = version;
    readme_json["size"] = size;
    readme_json["epochs"] = epochs;
    readme_json["comment"] = comment;
    readme_json["created_at"] = ss.str();
    readme_json["auroc"] = auroc_value;
    readme_json["f1_score"] = f1_value;

    fs::create_directories(AppPaths::WIP_Model_Path);
    std::ofstream out(AppPaths::WIP_Model_Path / "model.readme");
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
        int total_training_images_count = 0;
        int included_training_images_count = 0;

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
                    std::string img_id = entry["img_id"];
                    std::string src_img_path = entry["src_img_path"];
                    std::string dest_img_path = entry["dest_img_path"];
                    std::string source_name = entry["source_name"];
                    std::string source_type = entry["source_type"];
                    std::string category = entry["category"];
                    std::string inclusion = entry["inclusion"];
                    std::string dataset_type = entry["dataset_type"];

                    // Filter based on dataset type
                    if (dataset_type != "train")
                        continue;
                    
                    total_training_images_count++;

                    if (inclusion == "Included")
                        included_training_images_count++;
                    
                    // Filter based on inclusion status
                    if (selected_img_inclusion != inclusion)
                        continue;

                    // Filter based on category
                    if (selected_img_category != category)
                        continue;

                    images_from_training_set.emplace_back(ImageInfo {
                        img_id,
                        src_img_path,
                        dest_img_path,
                        source_name,
                        source_type,
                        category,
                        inclusion,
                        dataset_type
                    });
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error while scanning images: " << e.what() << '\n';
        }

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this, imgs = std::move(images_from_training_set), included_training_images_count, total_training_images_count]() {
            populate_wizard_train_images_listbox(imgs);
            m_toggle_all_on_wizard_btn->set_label("Select All");
            m_selected_images_on_wizard_listbox.clear();
            
            // Update the count labels
            m_wizard_training_selected_count_lbl->set_text("0");
            m_wizard_training_included_count_lbl->set_text(std::to_string(included_training_images_count));
            m_wizard_training_total_count_lbl->set_text(std::to_string(total_training_images_count));
            
            // Enable the refresh button again
            m_training_wizard_image_refresh_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::populate_wizard_train_images_listbox(const std::vector<ImageInfo>& images)
{
    // clear previous rows
    for (auto* child : m_wizard_train_images_lbox->get_children())
    {
        m_wizard_train_images_lbox->remove(*child);
    }

    // add one row per image
    for (const auto& info : images)
    {
        auto filename = info.src_img_path.filename().string();

        // Outer vertical box for header and details
        auto vbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);

        // Top row: checkbox + filename label + "Info" button
        auto hbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);

        // Checkbox
        auto checkbox = Gtk::make_managed<Gtk::CheckButton>();
        checkbox->set_halign(Gtk::Align::ALIGN_CENTER);
        hbox->pack_start(*checkbox, Gtk::PACK_SHRINK);
        checkbox->signal_toggled().connect([this, checkbox, info]() {
            bool is_checked = checkbox->get_active();
            if (is_checked)
                m_selected_images_on_wizard_listbox[checkbox] = info.img_id;
            else
                m_selected_images_on_wizard_listbox.erase(checkbox);

            // Update the count label
            m_wizard_training_selected_count_lbl->set_text(std::to_string(m_selected_images_on_wizard_listbox.size()));
        });

        auto lbl = Gtk::make_managed<Gtk::Label>(filename);
        lbl->set_xalign(0);
        lbl->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        lbl->set_tooltip_text(filename);
        lbl->set_max_width_chars(40);
        lbl->set_single_line_mode(true);

        hbox->pack_start(*lbl, Gtk::PACK_EXPAND_WIDGET);

        auto toggle_btn = Gtk::make_managed<ExpandCollapseButton>();
        hbox->pack_start(*toggle_btn, Gtk::PACK_SHRINK);

        // Detail content
        auto details_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);
        auto dataset_type_label = Gtk::make_managed<Gtk::Label>("Dataset Type" + std::string(": ") + info.dataset_type);
        dataset_type_label->set_xalign(0);
        auto category_label = Gtk::make_managed<Gtk::Label>("Category" + std::string(": ") + info.category);
        category_label->set_xalign(0);
        auto source_name_label = Gtk::make_managed<Gtk::Label>("Data Source Name" + std::string(": ") + info.source_name);
        source_name_label->set_xalign(0);
        auto source_type_label = Gtk::make_managed<Gtk::Label>("Data Source Type" + std::string(": ") + info.source_type);
        source_type_label->set_xalign(0);
        auto img_path_label = Gtk::make_managed<Gtk::Label>("Image Path" + std::string(": ") + info.dest_img_path.string());
        img_path_label->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        img_path_label->set_max_width_chars(40); // Limit display width
        img_path_label->set_tooltip_text(info.dest_img_path.string());
        img_path_label->set_xalign(0); // Align left
        details_box->pack_start(*dataset_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*category_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_name_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*img_path_label, Gtk::PACK_SHRINK);
        details_box->set_margin_start(5);
        details_box->set_margin_end(5);
        details_box->set_margin_top(5);
        details_box->set_margin_bottom(5);

        // Wrap detail box in a Revealer
        auto revealer = Gtk::make_managed<Gtk::Revealer>();
        revealer->set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
        revealer->set_transition_duration(200);
        revealer->add(*details_box);
        revealer->set_reveal_child(false);  // initially hidden

        // Toggle the Revealer when the button is clicked
        toggle_btn->signal_toggled.connect([revealer](bool expanded) {
            revealer->set_reveal_child(expanded);
        });

        // Pack into vertical container
        vbox->pack_start(*hbox, Gtk::PACK_SHRINK);
        vbox->pack_start(*revealer, Gtk::PACK_SHRINK);

        // Create row and add to listbox
        auto row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->add(*vbox);

        // Store the path as custom data
        row->set_data("image_path", new fs::path(info.dest_img_path));

        // Add the row to the listbox
        m_wizard_train_images_lbox->append(*row);
    }
    m_wizard_train_images_lbox->show_all_children();
}

void MainWindow::update_selected_images_inclusion(const std::string& inclusion)
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
    for (const auto& [checkbox, img_id] : m_selected_images_on_wizard_listbox)
    {
        // Check if the image is already in the dataset
        auto it = std::find_if(images_json.begin(), images_json.end(),
            [img_id](const json& entry) {
                return entry["img_id"] == img_id;
            });
        if (it != images_json.end())
        {
            // Image found, update its inclusion status
            (*it)["inclusion"] = inclusion;
        }
        else
        {
            // Image doesn't exist, add it to the dataset
            std::cerr << "Image ID not found in dataset.json: " << img_id << std::endl;
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

    // Train model in a separate thread
    std::thread([this]() {
        if (m_model_name.empty())
        {
            std::cerr << "Model name cannot be empty." << std::endl;
            return;
        }

        prepare_wip_dataset("train");
        m_model_size = m_model_size_cbox->get_active_id();
        int max_epochs = m_max_epochs_sbtn->get_value_as_int();
        std::string model_ckpt = "";
        if (m_select_model_rbtn->get_active())
        {
            std::string current_version = m_model_version_cbox->get_active_text();
            model_ckpt = AppPaths::Models_Path/m_model_name/current_version/"model.ckpt";
        }

        run_train_efficient_ad_model_script(m_model_name, m_model_size, max_epochs, model_ckpt);
        
        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this]() {
            m_train_model_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::prepare_wip_dataset(std::string dataset_type)
{
    try
    {
        auto dataset_path = AppPaths::WIP_Dataset_Path / dataset_type;
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
        auto dataset_json = AppPaths::WIP_Dataset_Path / "dataset.json";
        if (!fs::exists(dataset_json))
        {
            std::cerr << dataset_json << " does not exist" << std::endl;
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
                    std::string entry_dataset_type = entry["dataset_type"];

                    // Filter based on inclusion status
                    if (inclusion != "Included")
                        continue;

                    // Filter based on dataset type
                    if (entry_dataset_type != dataset_type)
                        continue;

                    // Copy the image to the appropriate directory
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

bool MainWindow::run_train_efficient_ad_model_script(const std::string& model_name, const std::string& model_size, int max_epochs, const std::string& model_ckpt)
{
    Glib::RefPtr<Gtk::TextBuffer> buffer = m_train_model_tview->get_buffer();
    std::array<char, 256> buffer_line;

    fs::path script_path = AppPaths::WIP_Path / "train_model.py";
    std::string cmd =
        "bash -c 'source ~/anaconda3/etc/profile.d/conda.sh && "
        "conda activate eagle_nest && "
        "python \"" + script_path.string() + "\" " + model_name + " " + model_size + " " + std::to_string(max_epochs) + " " + model_ckpt +
        " 2>&1'";  // <-- This redirects stderr to stdout;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        buffer->insert(buffer->end(), "Failed to start script.\n");
        return false;
    }

    // char buffer_line[256];
    while (fgets(buffer_line.data(), buffer_line.size(), pipe) != nullptr) {
        std::string line(buffer_line.data());
        Glib::signal_idle().connect_once([buffer, line]() {
            buffer->insert(buffer->end(), line);
        });
    }

    int status = pclose(pipe); // Blocks until script finishes
    int exit_code = 1;

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            buffer->insert(buffer->end(), "\nTraining failed with exit code: " + std::to_string(exit_code) + "\n");
        }
    } else {
        buffer->insert(buffer->end(), "\nTraining process did not exit normally.\n");
    }

    return exit_code == 0; // Return true if the script executed successfully
}

void MainWindow::on_test_model_clicked()
{
    // Disable the button to prevent multiple clicks
    m_test_model_btn->set_sensitive(false);

    // Test model in a separate thread
    std::thread([this]() {
        if (m_model_name.empty())
        {
            std::cerr << "Model name cannot be empty." << std::endl;
            return;
        }

        std::string model_ckpt = AppPaths::WIP_Model_Path/"EfficientAd"/m_model_name/"latest"/"weights"/"lightning"/"model.ckpt";
        bool result = run_test_efficient_ad_model_script(m_model_name, m_model_version, model_ckpt, m_auroc_value, m_f1_value);

        if (result) {
            auto dest_json = AppPaths::WIP_Testset_Path / m_model_name / m_model_version / "pred_results.json";

            load_prediction_results(dest_json.string(), m_image_to_heatmap_map);
        }

        std::vector<ImagePrediction> images_from_testing_set;
        if (result) {
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
                    std::string img_id = entry["img_id"];
                    std::string src_img_path = entry["src_img_path"];
                    std::string dest_img_path = entry["dest_img_path"];
                    std::string source_name = entry["source_name"];
                    std::string source_type = entry["source_type"];
                    std::string category = entry["category"];
                    std::string inclusion = entry["inclusion"];
                    std::string dataset_type = entry["dataset_type"];
                    const auto& heatmap_info = m_image_to_heatmap_map[dest_img_path];
                    std::string heatmap_path = heatmap_info.first;
                    float anomaly_score = heatmap_info.second;

                    // Filter based on dataset type
                    if (dataset_type != "test")
                        continue;

                    // Filter based on inclusion
                    if (inclusion != "Included")
                        continue;

                    images_from_testing_set.emplace_back(ImagePrediction {
                        img_id,
                        src_img_path,
                        dest_img_path,
                        source_name,
                        source_type,
                        category,
                        inclusion,
                        dataset_type,
                        heatmap_path,
                        anomaly_score
                    });
                }
            }
        }

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this, result, imgs = std::move(images_from_testing_set)]() {
            m_test_model_btn->set_sensitive(true);
            if (result)
            {
                m_f1_score_lbl->set_text(std::to_string(m_f1_value));
                m_area_under_roc_lbl->set_text(std::to_string(m_auroc_value));
                populate_testing_images_listbox(*m_wizard_test_images_lbox, imgs);
                fs::path score_distr_path = AppPaths::WIP_Testset_Path / m_model_name / m_model_version / "score_distribution.png";
                auto pixbuf = Gdk::Pixbuf::create_from_file(score_distr_path.string());
                if (!pixbuf) {
                    std::cerr << "Failed to load score distribution image." << std::endl;
                } else {
                    m_wizard_anomaly_score_dist_img_widget->set(pixbuf);
                }
            }
        });
    }).detach(); // Detach the thread to allow it to run independently
}

bool MainWindow::run_test_efficient_ad_model_script(const std::string& model_name, const std::string& model_version, const std::string& model_ckpt, double& out_auroc_value, double& out_f1_value)
{
    Glib::RefPtr<Gtk::TextBuffer> buffer = m_train_model_tview->get_buffer();
    std::array<char, 256> buffer_line;

    fs::path script_path = AppPaths::WIP_Path / "test_model.py";
    std::string cmd =
        "bash -c 'source ~/anaconda3/etc/profile.d/conda.sh && "
        "conda activate eagle_nest && "
        "python \"" + script_path.string() + "\" " + model_name + " " + model_version + " " + model_ckpt +
        " 2>&1'";  // <-- This redirects stderr to stdout;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        buffer->insert(buffer->end(), "Failed to start script.\n");
        return false;
    }

    while (fgets(buffer_line.data(), buffer_line.size(), pipe) != nullptr) {
        std::string line(buffer_line.data());
        std::cout << line << std::endl; // Print to console for debugging

        // Extract AUROC and F1 Score values using regex
        std::smatch match;
        if (std::regex_search(line, match, std::regex("AUROC:\\s*([0-9.]+)"))) {
            out_auroc_value = std::stod(match[1].str());  // Convert to double
            out_auroc_value = std::round(out_auroc_value * 1000.0) / 1000.0; // Round to 3 decimal places
        } else if (std::regex_search(line, match, std::regex("F1 Score:\\s*([0-9.]+)"))) {
            out_f1_value = std::stod(match[1].str());     // Convert to double
            out_f1_value = std::round(out_f1_value * 1000.0) / 1000.0; // Round to 3 decimal places
        }

        Glib::signal_idle().connect_once([buffer, line]() {
            buffer->insert(buffer->end(), line);
        });
    }

    int status = pclose(pipe); // Blocks until script finishes
    int exit_code = 1;

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            buffer->insert(buffer->end(), "\nTesting failed with exit code: " + std::to_string(exit_code) + "\n");
        }
    } else {
        buffer->insert(buffer->end(), "\nTesting process did not exit normally.\n");
    }

    return exit_code == 0; // Return true if the script executed successfully
}

void MainWindow::populate_testing_images_listbox(Gtk::ListBox& listbox, const std::vector<ImagePrediction>& images, const bool show_anomaly_score)
{
    // clear previous rows
    for (auto* child : listbox.get_children())
    {
        listbox.remove(*child);
    }

    // add one row per image
    for (const auto& info : images)
    {
        auto filename = info.src_img_path.filename().string();

        // Outer vertical box for header and details
        auto vbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);

        // Top row: filename label + "Info" button
        auto hbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 6);

        auto lbl = Gtk::make_managed<Gtk::Label>(filename);
        lbl->set_xalign(0);
        lbl->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        lbl->set_tooltip_text(filename);
        lbl->set_max_width_chars(40);
        lbl->set_single_line_mode(true);

        hbox->pack_start(*lbl, Gtk::PACK_EXPAND_WIDGET);

        auto toggle_btn = Gtk::make_managed<ExpandCollapseButton>();
        hbox->pack_start(*toggle_btn, Gtk::PACK_SHRINK);

        // Detail content
        auto details_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 3);
        auto dataset_type_label = Gtk::make_managed<Gtk::Label>("Dataset Type" + std::string(": ") + info.dataset_type);
        dataset_type_label->set_xalign(0);
        auto category_label = Gtk::make_managed<Gtk::Label>("Category" + std::string(": ") + info.category);
        category_label->set_xalign(0);
        auto source_name_label = Gtk::make_managed<Gtk::Label>("Data Source Name" + std::string(": ") + info.source_name);
        source_name_label->set_xalign(0);
        auto source_type_label = Gtk::make_managed<Gtk::Label>("Data Source Type" + std::string(": ") + info.source_type);
        source_type_label->set_xalign(0);
        auto img_path_label = Gtk::make_managed<Gtk::Label>("Image Path" + std::string(": ") + info.dest_img_path.string());
        img_path_label->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        img_path_label->set_max_width_chars(40); // Limit display width
        img_path_label->set_tooltip_text(info.dest_img_path.string());
        img_path_label->set_xalign(0); // Align left
        if (show_anomaly_score)
        {
            auto anomaly_score_label = Gtk::make_managed<Gtk::Label>("Anomaly Score" + std::string(": ") + std::to_string(info.anomaly_score));
            anomaly_score_label->set_xalign(0);
            details_box->pack_start(*anomaly_score_label, Gtk::PACK_SHRINK);
        }
        details_box->pack_start(*dataset_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*category_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_name_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*source_type_label, Gtk::PACK_SHRINK);
        details_box->pack_start(*img_path_label, Gtk::PACK_SHRINK);
        details_box->set_margin_start(5);
        details_box->set_margin_end(5);
        details_box->set_margin_top(5);
        details_box->set_margin_bottom(5);

        // Wrap detail box in a Revealer
        auto revealer = Gtk::make_managed<Gtk::Revealer>();
        revealer->set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
        revealer->set_transition_duration(200);
        revealer->add(*details_box);
        revealer->set_reveal_child(false);  // initially hidden

        // Toggle the Revealer when the button is clicked
        toggle_btn->signal_toggled.connect([revealer](bool expanded) {
            revealer->set_reveal_child(expanded);
        });

        // Pack into vertical container
        vbox->pack_start(*hbox, Gtk::PACK_SHRINK);
        vbox->pack_start(*revealer, Gtk::PACK_SHRINK);

        // Create row and add to listbox
        auto row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->add(*vbox);

        // Store the path as custom data
        row->set_data("image_path", new fs::path(info.dest_img_path));

        // Add the row to the listbox
        listbox.append(*row);
    }
    listbox.show_all_children();

    // Select the first row if no row is selected
    if (!listbox.get_selected_row()) {
        const auto& children = listbox.get_children();
        if (!children.empty()) {
            if (auto* first = dynamic_cast<Gtk::ListBoxRow*>(children.front())) {
                listbox.select_row(*first);
                activate_eval_testing_images_row(first);
            }
        }
    }
}

void MainWindow::load_prediction_results(
    const std::string& json_path,
    std::unordered_map<std::string, std::pair<std::string, float>>& image_to_heatmap_map)
{
    image_to_heatmap_map.clear();
    std::ifstream ifs(json_path);
    if (!ifs) {
        std::cerr << "Failed to open pred_results.json at " << json_path << std::endl;
        return;
    }

    nlohmann::json results;
    ifs >> results;

    for (const auto& entry : results) {
        std::string img_path = entry["input_image"];
        std::string heatmap_path = entry["anomaly_heatmap"];
        float anomaly_score = entry["anomaly_score"];
        image_to_heatmap_map[img_path] = {heatmap_path, anomaly_score};
    }
}

void MainWindow::on_save_model_clicked()
{
    // Disable the button to prevent multiple clicks
    m_save_model_btn->set_sensitive(false);

    // Save model in a separate thread
    std::thread([this]() {
        std::string comment = m_model_comment_to_save_tview->get_buffer()->get_text();
        std::string model_size = m_model_size_cbox->get_active_id();
        int max_epochs = m_max_epochs_sbtn->get_value_as_int();

        bool result = convert_efficient_ad_model_to_onnx(m_model_name);
        if (result) {
            write_model_readme(m_model_name, m_model_version, model_size, max_epochs, comment, m_auroc_value, m_f1_value);

            auto dataset_path = AppPaths::WIP_Dataset_Path/"dataset.json";
            auto model_ckpt_path = AppPaths::WIP_Model_Path/"EfficientAd"/m_model_name/"latest"/"weights"/"lightning"/"model.ckpt";
            auto model_onnx_path = AppPaths::WIP_Model_Path/"EfficientAd"/m_model_name/"latest"/"weights"/"onnx"/"model.onnx";
            auto model_readme_path = AppPaths::WIP_Model_Path/"model.readme";
            auto model_dest_path = AppPaths::Models_Path/m_model_name/m_model_version;

            // Create the model directory if it doesn't exist
            fs::create_directories(model_dest_path);
            // Copy the dataset.json to AppPaths::Models_Path
            fs::copy(dataset_path, model_dest_path/"dataset.json", fs::copy_options::overwrite_existing);
            // Copy the model files to AppPaths::Models_Path
            fs::copy(model_ckpt_path, model_dest_path/"model.ckpt", fs::copy_options::overwrite_existing);
            fs::copy(model_onnx_path, model_dest_path/"model.onnx", fs::copy_options::overwrite_existing);
            fs::copy(model_readme_path, model_dest_path/"model.readme", fs::copy_options::overwrite_existing);
        }

        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this]() {
            m_save_model_btn->set_sensitive(true);
        });
    }).detach(); // Detach the thread to allow it to run independently
}

bool MainWindow::convert_efficient_ad_model_to_onnx(const std::string& model_name)
{
    Glib::RefPtr<Gtk::TextBuffer> buffer = m_train_model_tview->get_buffer();
    std::array<char, 256> buffer_line;

    fs::path script_path = AppPaths::WIP_Path / "convert_model.py";
    std::string cmd =
        "bash -c 'source ~/anaconda3/etc/profile.d/conda.sh && "
        "conda activate eagle_nest && "
        "python \"" + script_path.string() + "\" " + model_name +
        " 2>&1'";  // <-- This redirects stderr to stdout;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        buffer->insert(buffer->end(), "Failed to start script.\n");
        return false;
    }

    while (fgets(buffer_line.data(), buffer_line.size(), pipe) != nullptr) {
        std::string line(buffer_line.data());
        Glib::signal_idle().connect_once([buffer, line]() {
            buffer->insert(buffer->end(), line);
        });
    }

    int status = pclose(pipe); // Blocks until script finishes
    int exit_code = 1;

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            buffer->insert(buffer->end(), "\nTraining failed with exit code: " + std::to_string(exit_code) + "\n");
        }
    } else {
        buffer->insert(buffer->end(), "\nTraining process did not exit normally.\n");
    }

    return exit_code == 0; // Return true if the script executed successfully
}

void MainWindow::on_model1_existing_models_selection_changed()
{
    std::string selected_model = m_model1_existing_models_cbox->get_active_text();
    if (selected_model.empty())
        return;

    // Clear the model version combo box
    m_model1_version_cbox->remove_all();

    // Populate the model version combo box with available versions
    auto model_path = AppPaths::Models_Path / selected_model;
    if (fs::exists(model_path) && fs::is_directory(model_path))
    {
        for (const auto& entry : fs::directory_iterator(model_path))
        {
            if (entry.is_directory())
            {
                std::string version = entry.path().filename().string();
                m_model1_version_cbox->append(version);
            }
        }
    }
}

void MainWindow::on_model1_version_selection_changed()
{
    std::string selected_model = m_model1_existing_models_cbox->get_active_text();
    std::string selected_version = m_model1_version_cbox->get_active_text();

    if (selected_model.empty() || selected_version.empty())
        return;

    // Load the model readme file and populate the comment text view
    auto model_path = AppPaths::Models_Path / selected_model / selected_version / "model.readme";
    if (fs::exists(model_path))
    {
        std::ifstream ifs(model_path);
        if (ifs)
        {
            json readme_json;
            ifs >> readme_json;
            ifs.close();

            // Populate the model comment text view
            m_model1_comment_tview->get_buffer()->set_text(readme_json["comment"]);
        }
    }
    else
    {
        std::cerr << "Model readme file not found." << std::endl;
    }
}

void MainWindow::on_model1_anomaly_score_dist_zoom_in_clicked()
{
    if (!m_model1_anomaly_score_dist_pixbuf)
    {
        std::cerr << "No image loaded for zooming." << std::endl;
        return;
    }

    // Zoom in
    const double zoom_step = 0.1;
    m_model1_anomaly_score_dist_zoom_scale *= (1.0 + zoom_step);

    // Limit zoom factor
    m_model1_anomaly_score_dist_zoom_scale = std::clamp(m_model1_anomaly_score_dist_zoom_scale, 0.1, 10.0);

    int new_width = m_model1_anomaly_score_dist_pixbuf->get_width() * m_model1_anomaly_score_dist_zoom_scale;
    int new_height = m_model1_anomaly_score_dist_pixbuf->get_height() * m_model1_anomaly_score_dist_zoom_scale;

    auto zoomed_pixbuf = m_model1_anomaly_score_dist_pixbuf->scale_simple(
        new_width, new_height, Gdk::INTERP_BILINEAR);

    m_model1_anomaly_score_dist_img_widget->set(zoomed_pixbuf);
}

void MainWindow::on_model1_anomaly_score_dist_zoom_out_clicked()
{
    if (!m_model1_anomaly_score_dist_pixbuf)
    {
        std::cerr << "No image loaded for zooming." << std::endl;
        return;
    }
    
    // Zoom out
    const double zoom_step = 0.1;
    m_model1_anomaly_score_dist_zoom_scale /= (1.0 + zoom_step);

    // Limit zoom factor
    m_model1_anomaly_score_dist_zoom_scale = std::clamp(m_model1_anomaly_score_dist_zoom_scale, 0.1, 10.0);

    int new_width = m_model1_anomaly_score_dist_pixbuf->get_width() * m_model1_anomaly_score_dist_zoom_scale;
    int new_height = m_model1_anomaly_score_dist_pixbuf->get_height() * m_model1_anomaly_score_dist_zoom_scale;

    auto zoomed_pixbuf = m_model1_anomaly_score_dist_pixbuf->scale_simple(
        new_width, new_height, Gdk::INTERP_BILINEAR);

    m_model1_anomaly_score_dist_img_widget->set(zoomed_pixbuf);
}

bool MainWindow::on_model1_anomaly_score_dist_image_scroll(GdkEventScroll* event)
{
    const double zoom_step = 0.1;

    if (event->direction == GDK_SCROLL_UP || (event->direction == GDK_SCROLL_SMOOTH && event->delta_y < 0))
        m_model1_anomaly_score_dist_zoom_scale *= (1.0 + zoom_step); // Zoom in
    else if (event->direction == GDK_SCROLL_DOWN || (event->direction == GDK_SCROLL_SMOOTH && event->delta_y > 0))
        m_model1_anomaly_score_dist_zoom_scale /= (1.0 + zoom_step); // Zoom out
    else
        return false; // Not handled

    // Limit zoom factor
    m_model1_anomaly_score_dist_zoom_scale = std::clamp(m_model1_anomaly_score_dist_zoom_scale, 0.1, 10.0);

    int new_width = m_model1_anomaly_score_dist_pixbuf->get_width() * m_model1_anomaly_score_dist_zoom_scale;
    int new_height = m_model1_anomaly_score_dist_pixbuf->get_height() * m_model1_anomaly_score_dist_zoom_scale;

    auto zoomed_pixbuf = m_model1_anomaly_score_dist_pixbuf->scale_simple(
        new_width, new_height, Gdk::INTERP_BILINEAR);

    m_model1_anomaly_score_dist_img_widget->set(zoomed_pixbuf);

    return true; // Event handled
}

void MainWindow::on_model2_existing_models_selection_changed()
{
    std::string selected_model = m_model2_existing_models_cbox->get_active_text();
    if (selected_model.empty())
        return;

    // Clear the model version combo box
    m_model2_version_cbox->remove_all();

    // Populate the model version combo box with available versions
    auto model_path = AppPaths::Models_Path / selected_model;
    if (fs::exists(model_path) && fs::is_directory(model_path))
    {
        for (const auto& entry : fs::directory_iterator(model_path))
        {
            if (entry.is_directory())
            {
                std::string version = entry.path().filename().string();
                m_model2_version_cbox->append(version);
            }
        }
    }
}

void MainWindow::on_model2_version_selection_changed()
{
    std::string selected_model = m_model2_existing_models_cbox->get_active_text();
    std::string selected_version = m_model2_version_cbox->get_active_text();

    if (selected_model.empty() || selected_version.empty())
        return;

    // Load the model readme file and populate the comment text view
    auto model_path = AppPaths::Models_Path / selected_model / selected_version / "model.readme";
    if (fs::exists(model_path))
    {
        std::ifstream ifs(model_path);
        if (ifs)
        {
            json readme_json;
            ifs >> readme_json;
            ifs.close();

            // Populate the model comment text view
            m_model2_comment_tview->get_buffer()->set_text(readme_json["comment"]);
        }
    }
    else
    {
        std::cerr << "Model readme file not found." << std::endl;
    }
}

bool MainWindow::on_model2_anomaly_score_dist_image_scroll(GdkEventScroll* event)
{
    const double zoom_step = 0.1;

    if (event->direction == GDK_SCROLL_UP || (event->direction == GDK_SCROLL_SMOOTH && event->delta_y < 0))
        m_model2_anomaly_score_dist_zoom_scale *= (1.0 + zoom_step); // Zoom in
    else if (event->direction == GDK_SCROLL_DOWN || (event->direction == GDK_SCROLL_SMOOTH && event->delta_y > 0))
        m_model2_anomaly_score_dist_zoom_scale /= (1.0 + zoom_step); // Zoom out
    else
        return false; // Not handled

    // Limit zoom factor
    m_model2_anomaly_score_dist_zoom_scale = std::clamp(m_model2_anomaly_score_dist_zoom_scale, 0.1, 10.0);

    int new_width = m_model2_anomaly_score_dist_pixbuf->get_width() * m_model2_anomaly_score_dist_zoom_scale;
    int new_height = m_model2_anomaly_score_dist_pixbuf->get_height() * m_model2_anomaly_score_dist_zoom_scale;

    auto zoomed_pixbuf = m_model2_anomaly_score_dist_pixbuf->scale_simple(
        new_width, new_height, Gdk::INTERP_BILINEAR);

    m_model2_anomaly_score_dist_img_widget->set(zoomed_pixbuf);

    return true; // Event handled
}

void MainWindow::on_eval_model_clicked()
{
    // Disable the button to prevent multiple clicks
    m_eval_model_btn->set_sensitive(false);

    // Evaluate model in a separate thread
    std::thread([this]() {
        // Ensure the dataset directory exists
        auto dataset_dir = AppPaths::WIP_Dataset_Path;
        if (!fs::exists(dataset_dir))
        {
            fs::create_directories(dataset_dir);
        }

        // If dest_json does not exist, copy it from the source
        auto dest_json = dataset_dir / "dataset.json";
        if (!fs::exists(dest_json))
        {
            auto source_json = AppPaths::Dataset_Path / "dataset.json";
            fs::copy(source_json, dest_json, fs::copy_options::overwrite_existing);
        }

        bool result1 = false;
        bool result2 = false;
        std::string model1_name = m_model1_existing_models_cbox->get_active_text();
        std::string model1_version = m_model1_version_cbox->get_active_text();
        std::string model2_name = m_model2_existing_models_cbox->get_active_text();
        std::string model2_version = m_model2_version_cbox->get_active_text();

        if (!model1_name.empty() && !model1_version.empty())
        {
            std::string model1_ckpt = AppPaths::Models_Path / model1_name / model1_version / "model.ckpt";
            result1 = run_test_efficient_ad_model_script(model1_name, model1_version, model1_ckpt, m_model1_auroc_value, m_model1_f1_value);    
        }
        
        if (!model2_name.empty() && !model2_version.empty())
        {
            std::string model2_ckpt = AppPaths::Models_Path / model2_name / model2_version / "model.ckpt";
            result2 = run_test_efficient_ad_model_script(model2_name, model2_version, model2_ckpt, m_model2_auroc_value, m_model2_f1_value);    
        }

        if (!result1 && !result2)
        {
            std::cerr << "Both models failed to evaluate." << std::endl;
            return;
        }

        // Load images from the dataset.json file
        std::ifstream ifs(dest_json);
        json images_json;

        if (!ifs)
        {
            std::cerr << "Failed to open dataset.json" << std::endl;
            return;
        }
        else
        {
            ifs >> images_json;
            ifs.close();
        }

        // Load prediction results for both models
        std::vector<ImagePrediction> model1_images_from_testing_set;
        std::vector<ImagePrediction> model2_images_from_testing_set;
        
        if (result1) {
            auto dest_json = AppPaths::WIP_Testset_Path / model1_name / model1_version / "pred_results.json";
            load_prediction_results(dest_json.string(), m_model1_image_to_heatmap_map);
        }

        if (result2) {
            auto dest_json = AppPaths::WIP_Testset_Path / model2_name / model2_version / "pred_results.json";
            load_prediction_results(dest_json.string(), m_model2_image_to_heatmap_map);
        }

        for (const auto& entry : images_json)
        {
            std::string img_id = entry["img_id"];
            std::string src_img_path = entry["src_img_path"];
            std::string dest_img_path = entry["dest_img_path"];
            std::string source_name = entry["source_name"];
            std::string source_type = entry["source_type"];
            std::string category = entry["category"];
            std::string inclusion = entry["inclusion"];
            std::string dataset_type = entry["dataset_type"];

            // Filter based on dataset type
            if (dataset_type != "test")
                continue;

            // Filter based on inclusion
            if (inclusion != "Included")
                continue;

            if (result1)
            {
                const auto& model1_heatmap_info = m_model1_image_to_heatmap_map[dest_img_path];
                std::string model1_heatmap_path = model1_heatmap_info.first;
                float model1_anomaly_score = model1_heatmap_info.second;
                model1_images_from_testing_set.emplace_back(ImagePrediction {
                    img_id,
                    src_img_path,
                    dest_img_path,
                    source_name,
                    source_type,
                    category,
                    inclusion,
                    dataset_type,
                    model1_heatmap_path,
                    model1_anomaly_score
                });
            }

            if (result2)
            {
                const auto& model2_heatmap_info = m_model2_image_to_heatmap_map[dest_img_path];
                std::string model2_heatmap_path = model2_heatmap_info.first;
                float model2_anomaly_score = model2_heatmap_info.second;
                model2_images_from_testing_set.emplace_back(ImagePrediction {
                    img_id,
                    src_img_path,
                    dest_img_path,
                    source_name,
                    source_type,
                    category,
                    inclusion,
                    dataset_type,
                    model2_heatmap_path,
                    model2_anomaly_score
                });
            }
        }
        
        // Once done, update the button in the UI thread
        Glib::signal_idle().connect_once([this, result1, result2, model1_imgs = std::move(model1_images_from_testing_set), model2_imgs = std::move(model2_images_from_testing_set)]() {
            m_eval_model_btn->set_sensitive(true);
            if (result1)
            {
                m_model1_f1_score_lbl->set_text(std::to_string(m_model1_f1_value));
                m_model1_area_under_roc_lbl->set_text(std::to_string(m_model1_auroc_value));
                std::string model1_name = m_model1_existing_models_cbox->get_active_text();
                std::string model1_version = m_model1_version_cbox->get_active_text();        
                fs::path score_dist_path = AppPaths::WIP_Testset_Path / model1_name / model1_version / "score_distribution.png";
                m_model1_anomaly_score_dist_pixbuf = Gdk::Pixbuf::create_from_file(score_dist_path.string());
                if (!m_model1_anomaly_score_dist_pixbuf) {
                    std::cerr << "Failed to load score distribution image." << std::endl;
                } else {
                    //
                    // Scale the image to fit the widget
                    //
                    
                    // Get original size of the pixbuf
                    int img_width = m_model1_anomaly_score_dist_pixbuf->get_width();
                    int img_height = m_model1_anomaly_score_dist_pixbuf->get_height();

                    // Get desired size from the Gtk::Image widget
                    int widget_width = m_model1_anomaly_score_dist_img_widget->get_allocated_width();
                    int widget_height = m_model1_anomaly_score_dist_img_widget->get_allocated_height();

                    // Fall back to original size if widget size is not ready
                    if (widget_width <= 1 || widget_height <= 1) {
                        widget_width = img_width;
                        widget_height = img_height;
                    }
                
                    // Compute scale factor to fit within widget
                    double scale = std::min(
                        (double)widget_width / img_width,
                        (double)widget_height / img_height
                    );
                
                    int scaled_width = static_cast<int>(img_width * scale);
                    int scaled_height = static_cast<int>(img_height * scale);

                    // Scale the image
                    auto scaled_pixbuf = m_model1_anomaly_score_dist_pixbuf->scale_simple(scaled_width, scaled_height, Gdk::INTERP_BILINEAR);

                    // Set it to the widget
                    m_model1_anomaly_score_dist_img_widget->set(scaled_pixbuf);                
                }
            }
            
            if (result2)
            {
                m_model2_f1_score_lbl->set_text(std::to_string(m_model2_f1_value));
                m_model2_area_under_roc_lbl->set_text(std::to_string(m_model2_auroc_value));
                std::string model2_name = m_model2_existing_models_cbox->get_active_text();
                std::string model2_version = m_model2_version_cbox->get_active_text();        
                fs::path score_dist_path = AppPaths::WIP_Testset_Path / model2_name / model2_version / "score_distribution.png";
                m_model2_anomaly_score_dist_pixbuf = Gdk::Pixbuf::create_from_file(score_dist_path.string());
                if (!m_model2_anomaly_score_dist_pixbuf) {
                    std::cerr << "Failed to load score distribution image." << std::endl;
                } else {
                    //
                    // Scale the image to fit the widget
                    //

                    // Get original size of the pixbuf
                    int img_width = m_model2_anomaly_score_dist_pixbuf->get_width();
                    int img_height = m_model2_anomaly_score_dist_pixbuf->get_height();

                    // Get desired size from the Gtk::Image widget
                    int widget_width = m_model2_anomaly_score_dist_img_widget->get_allocated_width();
                    int widget_height = m_model2_anomaly_score_dist_img_widget->get_allocated_height();

                    // Fall back to original size if widget size is not ready
                    if (widget_width <= 1 || widget_height <= 1) {
                        widget_width = img_width;
                        widget_height = img_height;
                    }
                
                    // Compute scale factor to fit within widget
                    double scale = std::min(
                        (double)widget_width / img_width,
                        (double)widget_height / img_height
                    );
                
                    int scaled_width = static_cast<int>(img_width * scale);
                    int scaled_height = static_cast<int>(img_height * scale);

                    // Scale the image
                    auto scaled_pixbuf = m_model2_anomaly_score_dist_pixbuf->scale_simple(scaled_width, scaled_height, Gdk::INTERP_BILINEAR);

                    m_model2_anomaly_score_dist_img_widget->set(scaled_pixbuf);
                }
            }

            if (result1 && !model1_imgs.empty())
            {
                populate_testing_images_listbox(*m_eval_test_images_lbox, model1_imgs, false);
            }
            else if (result2 && !model2_imgs.empty())
            {
                populate_testing_images_listbox(*m_eval_test_images_lbox, model2_imgs, false);
            }
            else
            {
                std::cerr << "No images found for evaluation." << std::endl;
            }
        });
    }).detach(); // Detach the thread to allow it to run independently
}

void MainWindow::activate_eval_testing_images_row(Gtk::ListBoxRow* row)
{
    if (!row) return;

    auto path_ptr = static_cast<fs::path*>(row->get_data("image_path"));
    if (!path_ptr) return;

    const fs::path& image_path = *path_ptr;

    load_image_and_heatmap_to_drawing_area(
        image_path.string(),
        m_model1_image_to_heatmap_map[image_path.string()].first,
        m_model1_test_img_pixbuf,
        m_model1_test_heatmap_pixbuf,
        m_model1_test_image_drawing_area
    );
    m_model1_test_img_anomaly_score_lbl->set_text(
        std::to_string(m_model1_image_to_heatmap_map[image_path.string()].second)
    );

    load_image_and_heatmap_to_drawing_area(
        image_path.string(),
        m_model2_image_to_heatmap_map[image_path.string()].first,
        m_model2_test_img_pixbuf,
        m_model2_test_heatmap_pixbuf,
        m_model2_test_image_drawing_area
    );
    m_model2_test_img_anomaly_score_lbl->set_text(
        std::to_string(m_model2_image_to_heatmap_map[image_path.string()].second)
    );

    std::cout << "Row activated for image: " << image_path << std::endl;
}

void MainWindow::on_eval_back_clicked()
{
    auto children = m_eval_test_images_lbox->get_children();
    Gtk::ListBoxRow* current = m_eval_test_images_lbox->get_selected_row();
    
    for (size_t i = 1; i < children.size(); ++i) {
        if (children[i] == current) {
            if (auto* prev = dynamic_cast<Gtk::ListBoxRow*>(children[i - 1])) {
                m_eval_test_images_lbox->select_row(*prev);
                activate_eval_testing_images_row(prev);
            }
            break;
        }
    }
}

void MainWindow::on_eval_next_clicked()
{
    auto children = m_eval_test_images_lbox->get_children();
    Gtk::ListBoxRow* current = m_eval_test_images_lbox->get_selected_row();
    
    for (size_t i = 0; i + 1 < children.size(); ++i) {
        if (children[i] == current) {
            if (auto* next = dynamic_cast<Gtk::ListBoxRow*>(children[i + 1])) {
                m_eval_test_images_lbox->select_row(*next);
                activate_eval_testing_images_row(next);
            }
            break;
        }
    }    
}