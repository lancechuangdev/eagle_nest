#ifndef EAGLE_EYE_MAIN_WINDOW_H
#define EAGLE_EYE_MAIN_WINDOW_H

#include <gtkmm.h>
#include <filesystem>

namespace fs = std::filesystem;

class MainWindow : public Gtk::Window
{
public:
    struct DatasetSource {
        // Index of the row in the UI grid, starting from 1 (excluding header row).
        // Set to -1 if the dataset should not be displayed in the UI.
        int dispaly_index;
        std::string name;
        std::string type;
        std::string connection_info;
        std::string connection_status;
    };
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder);
    virtual ~MainWindow();

protected:
    Gtk::RadioButton *m_model_btn;
    Gtk::RadioButton *m_explore_btn;
    Gtk::RadioButton *m_toolkit_btn;
    Gtk::Stack *m_content_stack;

    // Model Training Stack
    Gtk::Stack *m_training_stack;
    Gtk::Label *m_training_step1_lbl;
    Gtk::Label *m_training_step2_lbl;
    Gtk::Label *m_training_step3_lbl;
    Gtk::Button *m_previous_btn;
    Gtk::Button *m_next_btn;

    // Model Selection
    Gtk::RadioButton *m_create_model_rbtn;
    Gtk::Entry *m_model_name_entry;
    Gtk::RadioButton *m_select_model_rbtn;
    Gtk::ComboBoxText *m_existing_models_cbox;
    Gtk::ComboBoxText *m_model_version_cbox;
    Gtk::TextView *m_model_comment_tview;

    // Image Selection
    Gtk::RadioButton *m_select_from_dataset_sources_rbtn;
    Gtk::RadioButton *m_manual_selection_rbtn;
    Gtk::LinkButton *m_confirm_dataset_lbtn;
    
    // Dataset Explorer Stack
    Gtk::Stack *m_explorer_stack;
    Gtk::RadioButton *m_explorer_dataset_sources_rbtn;
    Gtk::RadioButton *m_explorer_images_from_sources_rbtn;
    Gtk::RadioButton *m_explorer_training_images_rbtn;
    Gtk::Button *m_dataset_sources_refresh_btn;
    Gtk::Grid *m_dataset_sources_grid;
    Gtk::ComboBoxText *m_dataset_sources_cbox;
    Gtk::ComboBoxText *m_image_category_cbox;
    Gtk::Button *m_images_from_sources_refresh_btn;
    Gtk::ListBox *m_explorer_images_lbox;

    void on_previous_clicked();
    void on_next_clicked();
    void on_dataset_sources_refresh_clicked();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    int m_current_step = 0;
    std::vector<std::string> m_training_page_names = {"page_select_model", "page_select_images", "page_training"};
    std::vector<Gtk::Label*> m_training_step_labels;
    std::vector<Gtk::CheckButton*> m_datasources_checkboxes;
    std::vector<std::pair<Gtk::Label*, Gtk::Label*>> m_datasources_connections;
    std::vector<Gtk::Label*> m_connection_status_labels;
    std::vector<DatasetSource> m_dataset_sources;

    void set_window_title(const std::string &title);
    void on_menu_toggled();
    void update_step_indicator();
    void transition_step(bool step_forward);
    void write_model_readme();
    void on_explorer_toggled();
    void discover_dataset_sources();
    void add_local_dataset_source(size_t datasource_id);
    void clear_dataset_sources();
    void add_dataset_sources_header();
    void add_dataset_source_row(size_t row_index, const std::string& name, const std::string& type, const std::string& connection_info, const std::string& connection_status, bool checked = true);
    void on_dataset_source_changed();
    void on_images_from_sources_refresh_clicked();
    void refresh_dataset_sources_options();
    void refresh_image_category_options();
    void populate_explorer_images_listbox(const std::vector<fs::path>& images);
    void on_img_add_clicked(const fs::path& image_path);
    void on_img_remove_clicked(const fs::path& image_path);
    void add_image_to_dataset(const fs::path& src_path, const std::string& category);
    void remove_image_from_dataset(const fs::path& src_path, const std::string& category);
};

#endif