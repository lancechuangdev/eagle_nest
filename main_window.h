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
    // Main menu
    Gtk::RadioButton *m_model_btn;
    Gtk::RadioButton *m_explore_btn;
    Gtk::Stack *m_content_stack;

    // Dataset Explorer Stack
    Gtk::Stack *m_explorer_stack;
    Gtk::RadioButton *m_explorer_dataset_sources_rbtn;
    Gtk::RadioButton *m_explorer_training_images_rbtn;
    Gtk::RadioButton *m_explorer_test_images_rbtn;

    // Dataset Explorer - Training Images
    Gtk::Button *m_dataset_sources_refresh_btn;
    Gtk::Grid *m_dataset_sources_grid;
    Gtk::ComboBoxText *m_dataset_sources_cbox;
    Gtk::ComboBoxText *m_train_image_category_cbox;
    Gtk::Button *m_train_images_refresh_btn;
    Gtk::ListBox *m_explorer_train_images_lbox;
    Gtk::DrawingArea *m_explorer_train_image_drawing_area;
    Gtk::Button *m_toggle_all_on_train_btn;
    Gtk::Button *m_add_train_image_btn;
    Gtk::Button *m_remove_train_image_btn;
    Gtk::Label *m_explorer_training_selected_count_lbl;
    Gtk::Label *m_explorer_training_total_count_lbl;

    // Dataset Explorer - Test Images
    Gtk::SpinButton *m_test_split_ratio_sbtn;
    Gtk::Button *m_auto_split_btn;
    Gtk::ComboBoxText *m_dataset_type_cbox;
    Gtk::ComboBoxText *m_test_image_category_cbox;
    Gtk::Button *m_test_images_refresh_btn;
    Gtk::ListBox *m_explorer_test_images_lbox;
    Gtk::DrawingArea *m_explorer_test_image_drawing_area;
    Gtk::Button *m_toggle_all_on_test_btn;
    Gtk::Button *m_add_test_image_btn;
    Gtk::Button *m_remove_test_image_btn;
    Gtk::Label *m_explorer_test_selected_count_lbl;
    Gtk::Label *m_explorer_test_total_count_lbl;

    // Model Training Startup
    Gtk::Button *m_start_train_model_btn;
    Gtk::Button *m_model_performance_evaluation_btn;

    // Model Training Stack
    Gtk::Stack *m_training_stack;
    Gtk::Label *m_training_step1_lbl;
    Gtk::Label *m_training_step2_lbl;
    Gtk::Label *m_training_step3_lbl;
    Gtk::Label *m_training_step4_lbl;
    Gtk::Label *m_training_step5_lbl;
    Gtk::Button *m_previous_btn;
    Gtk::Button *m_next_btn;
    Gtk::Button *m_close_training_wizard_btn;

    // Model Selection
    Gtk::RadioButton *m_create_model_rbtn;
    Gtk::Entry *m_model_name_entry;
    Gtk::RadioButton *m_select_model_rbtn;
    Gtk::ComboBoxText *m_existing_models_cbox;
    Gtk::ComboBoxText *m_model_version_cbox;
    Gtk::TextView *m_model_comment_tview;

    // Image Selection
    Gtk::ComboBoxText *m_training_wizard_img_included_cbox;
    Gtk::ComboBoxText *m_training_wizard_img_category_cbox;    
    Gtk::Button *m_training_wizard_image_refresh_btn;
    Gtk::DrawingArea *m_wizard_train_image_drawing_area;
    Gtk::ListBox *m_wizard_train_images_lbox;
    Gtk::Button *m_toggle_all_on_wizard_btn;
    Gtk::Button *m_include_train_images_btn;
    Gtk::Button *m_exclude_train_images_btn;
    Gtk::Label *m_wizard_training_selected_count_lbl;
    Gtk::Label *m_wizard_training_included_count_lbl;
    Gtk::Label *m_wizard_training_total_count_lbl;

    // Model Training
    Gtk::ComboBoxText *m_model_size_cbox;
    Gtk::SpinButton *m_max_epochs_sbtn;
    Gtk::Button *m_train_model_btn;
    Gtk::TextView *m_train_model_tview;

    // Model Testing
    Gtk::Label *m_model_under_test_lbl;
    Gtk::Label *m_model_version_under_test_lbl;
    Gtk::Button *m_test_model_btn;
    Gtk::Label *m_f1_score_lbl;
    Gtk::Label *m_area_under_roc_lbl;
    Gtk::ListBox *m_wizard_test_images_lbox;
    Gtk::DrawingArea *m_wizard_test_image_drawing_area;
    Gtk::Switch *m_wizard_show_anomaly_heatmap_switch;
    Gtk::Image *m_wizard_anomaly_score_dist_img_widget;

    // Save Model
    Gtk::Label *m_model_name_to_save_lbl;
    Gtk::Label *m_model_version_to_save_lbl;
    Gtk::Label *m_model_size_to_save_lbl;
    Gtk::TextView *m_model_comment_to_save_tview;
    Gtk::Button *m_save_model_btn;

    // Model Performance Evaluation
    Gtk::ComboBoxText *m_model1_existing_models_cbox;
    Gtk::ComboBoxText *m_model1_version_cbox;
    Gtk::TextView *m_model1_comment_tview;
    Gtk::Label *m_model1_f1_score_lbl;
    Gtk::Label *m_model1_area_under_roc_lbl;
    Gtk::Image *m_model1_anomaly_score_dist_img_widget;
    Gtk::Button *m_model1_anomaly_score_dist_zoom_in_btn;
    Gtk::Button *m_model1_anomaly_score_dist_zoom_out_btn;
    Gtk::DrawingArea *m_model1_test_image_drawing_area;
    Gtk::Button *m_model1_anomaly_heatmap_zoom_in_btn;
    Gtk::Button *m_model1_anomaly_heatmap_zoom_out_btn;
    Gtk::Label *m_model1_test_img_anomaly_score_lbl;

    Gtk::ComboBoxText *m_model2_existing_models_cbox;
    Gtk::ComboBoxText *m_model2_version_cbox;
    Gtk::TextView *m_model2_comment_tview;
    Gtk::Label *m_model2_f1_score_lbl;
    Gtk::Label *m_model2_area_under_roc_lbl;
    Gtk::Image *m_model2_anomaly_score_dist_img_widget;
    Gtk::Button *m_model2_anomaly_score_dist_zoom_in_btn;
    Gtk::Button *m_model2_anomaly_score_dist_zoom_out_btn;
    Gtk::DrawingArea *m_model2_test_image_drawing_area;
    Gtk::Button *m_model2_anomaly_heatmap_zoom_in_btn;
    Gtk::Button *m_model2_anomaly_heatmap_zoom_out_btn;
    Gtk::Label *m_model2_test_img_anomaly_score_lbl;

    Gtk::Button *m_eval_model_btn;
    Gtk::Switch *m_eval_show_anomaly_heatmap_switch;
    Gtk::Revealer *m_eval_test_images_list_revealer;
    Gtk::ListBox *m_eval_test_images_lbox;
    Gtk::Button *m_eval_back_btn;
    Gtk::Button *m_eval_next_btn;
    Gtk::Button *m_eval_placeholder_btn;

    // Key events
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

    // Mouse events
    void on_previous_clicked();
    void on_next_clicked();
    void on_close_training_wizard_clicked();
    void on_dataset_sources_refresh_clicked();
    void on_menu_toggled();
    void on_start_train_model_clicked();
    void on_model_performance_evaluation_clicked();
    void on_existing_model_selection_changed();
    void on_model_version_selection_changed();
    void on_training_wizard_image_refresh_clicked();
    void on_train_model_clicked();
    void on_test_model_clicked();
    void on_save_model_clicked();
    void on_explorer_toggled();
    void on_dataset_source_changed();
    void on_train_images_refresh_clicked();
    void on_add_train_images_clicked();
    void on_remove_train_images_clicked();
    bool on_image_draw(const Cairo::RefPtr<Cairo::Context>& cr,
        Glib::RefPtr<Gdk::Pixbuf> base_pixbuf,
        Glib::RefPtr<Gdk::Pixbuf> overlay_pixbuf,
        Gtk::DrawingArea* area,
        double overlay_alpha = 0.5f,
        double zoom_scale = 0.0f);
    void on_auto_split_clicked();
    void on_test_images_refresh_clicked();
    void on_add_test_images_clicked();
    void on_remove_test_images_clicked();
    void on_model1_existing_models_selection_changed();
    void on_model1_version_selection_changed();
    void on_model1_anomaly_score_dist_zoom_in_clicked();
    void on_model1_anomaly_score_dist_zoom_out_clicked();
    void on_model1_anomaly_heatmap_zoom_in_clicked();
    void on_model1_anomaly_heatmap_zoom_out_clicked();
    void on_model2_existing_models_selection_changed();
    void on_model2_version_selection_changed();
    void on_model2_anomaly_score_dist_zoom_in_clicked();
    void on_model2_anomaly_score_dist_zoom_out_clicked();
    void on_model2_anomaly_heatmap_zoom_in_clicked();
    void on_model2_anomaly_heatmap_zoom_out_clicked();
    void on_eval_model_clicked();
    void on_eval_back_clicked();
    void on_eval_next_clicked();
    
private:
    struct ImageInfo {
        std::string img_id; // Unique identifier for the image
        fs::path src_img_path; // Original path of the image
        fs::path dest_img_path; // Destination path of the image in the dataset
        std::string source_name;
        std::string source_type;
        std::string category; // Category of the image (e.g., "normal", "abnormal")
        std::string inclusion; // Flag indicating if the image is part of the training (e.g., "included", "excluded")
        std::string dataset_type; // Type of dataset (e.g., "train", "test")
    };

    struct ImagePrediction : public ImageInfo {
        fs::path heatmap_path;      // Path to the anomaly heatmap image
        float anomaly_score = 0.0f; // Anomaly score from the model
    };
    
    Glib::RefPtr<Gtk::Builder> m_builder;
    int m_current_step = 0;
    std::string m_active_model_page = "page_model_welcome";
    std::vector<std::string> m_training_page_names = {"page_select_model", "page_select_images", "page_training", "page_testing", "page_save_model"};
    std::vector<Gtk::Label*> m_training_step_labels;
    std::vector<Gtk::CheckButton*> m_datasources_checkboxes;
    std::vector<std::pair<Gtk::Label*, Gtk::Label*>> m_datasources_connections;
    std::vector<Gtk::Label*> m_connection_status_labels;
    std::vector<DatasetSource> m_dataset_sources;
    Glib::RefPtr<Gdk::Pixbuf> m_explorer_train_img_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_explorer_test_img_pixbuf;
    std::vector<ImageInfo> m_images_from_datasources;
    bool m_ctrl_pressed = false; // Flag to check if Ctrl key is pressed
    bool m_all_selected_on_train = false; // Flag to check if all images are selected in the training listbox 
    bool m_all_selected_on_test = false; // Flag to check if all images are selected in the test listbox
    bool m_all_selected_on_wizard = false; // Flag to check if all images are selected in the wizard listbox
    std::map<Gtk::CheckButton*, std::string> m_selected_images_on_train_listbox;
    std::map<Gtk::CheckButton*, std::string> m_selected_images_on_test_listbox;
    std::map<Gtk::CheckButton*, std::string> m_selected_images_on_wizard_listbox;
    std::string m_model_name;
    std::string m_model_version = "v1"; // The version of the model to be created
    std::string m_model_size; // The size of the model to be created
    Glib::RefPtr<Gdk::Pixbuf> m_wizard_train_img_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_wizard_test_img_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_wizard_test_heatmap_pixbuf;
    double m_wizard_anomaly_heatmap_zoom_scale = 1.0;
    bool m_wizard_show_heatmap = true;
    std::unordered_map<std::string, std::pair<std::string, float>> m_image_to_heatmap_map;
    double m_auroc_value = 0.0;
    double m_f1_value = 0.0;
    Glib::RefPtr<Gdk::Pixbuf> m_model1_anomaly_score_dist_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_model1_test_img_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_model1_test_heatmap_pixbuf;
    double m_model1_anomaly_score_dist_zoom_scale = 1.0;
    double m_model1_anomaly_heatmap_zoom_scale = 1.0;
    std::unordered_map<std::string, std::pair<std::string, float>> m_model1_image_to_heatmap_map;
    double m_model1_auroc_value = 0.0;
    double m_model1_f1_value = 0.0;
    Glib::RefPtr<Gdk::Pixbuf> m_model2_anomaly_score_dist_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_model2_test_img_pixbuf;
    Glib::RefPtr<Gdk::Pixbuf> m_model2_test_heatmap_pixbuf;
    double m_model2_anomaly_score_dist_zoom_scale = 1.0;
    double m_model2_anomaly_heatmap_zoom_scale = 1.0;
    std::unordered_map<std::string, std::pair<std::string, float>> m_model2_image_to_heatmap_map;
    double m_model2_auroc_value = 0.0;
    double m_model2_f1_value = 0.0;
    bool m_eval_show_heatmap = true;

    void set_window_title(const std::string &title);
    std::vector<std::string> get_existing_models();
    void update_step_indicator();
    void transition_step(bool step_forward);
    void write_model_readme(const std::string& name, const std::string& version, const std::string& size, const int epochs, const std::string& comment, const double auroc_value, const double f1_value);
    void populate_wizard_train_images_listbox(const std::vector<ImageInfo>& images);
    void update_selected_images_inclusion(const std::string& inclusion);
    void prepare_wip_dataset(std::string dataset_type);
    void prepare_wip_scripts();
    bool run_train_efficient_ad_model_script(const std::string& model_name, const std::string& model_size, int max_epochs, const std::string& model_ckpt);
    bool run_test_efficient_ad_model_script(const std::string& model_name, const std::string& model_version, const std::string& model_ckpt, double& out_auroc_value, double& out_f1_value);
    bool convert_efficient_ad_model_to_onnx(const std::string& model_name);
    void discover_dataset_sources();
    void add_local_dataset_source(size_t datasource_id);
    void clear_dataset_sources();
    void add_dataset_sources_header();
    void add_dataset_source_row(size_t row_index, const std::string& name, const std::string& type, const std::string& connection_info, const std::string& connection_status, bool checked = true);
    void refresh_dataset_sources_options();
    void populate_explorer_train_images_listbox();
    void add_selected_images_to_train(std::map<Gtk::CheckButton*, std::string> selected_images);
    void remove_selected_images_from_train(std::map<Gtk::CheckButton*, std::string> selected_images);
    fs::path copy_image_to_dataset(const fs::path& src_path, const std::string& dataset_type, const std::string& category);
    void remove_image_from_dataset(const fs::path& img_path);
    void load_image_to_drawing_area(const std::string& image_path, Glib::RefPtr<Gdk::Pixbuf>& pixbuf, Gtk::DrawingArea* target_drawing_area);
    void load_image_and_heatmap_to_drawing_area(
        const std::string& image_path,
        const std::string& heatmap_path,
        Glib::RefPtr<Gdk::Pixbuf>& image_pixbuf,
        Glib::RefPtr<Gdk::Pixbuf>& heatmap_pixbuf,
        Gtk::DrawingArea* target_drawing_area,
        double& updated_scale);
    void load_prediction_results(const std::string& json_path, std::unordered_map<std::string, std::pair<std::string, float>>& image_to_heatmap_map);
    void move_selected_images(std::map<Gtk::CheckButton*, std::string> selected_images, std::string dataset_type);
    std::string generate_sha256(const std::string& input);
    void populate_explorer_test_images_listbox(const std::vector<ImageInfo>& images);
    void set_all_checkboxes(Gtk::ListBox *images_lbox, bool checked);
    void populate_testing_images_listbox(Gtk::ListBox& listbox, const std::vector<ImagePrediction>& images, const bool show_anomaly_score = true);
    void activate_eval_testing_images_row(Gtk::ListBoxRow* row);
    Glib::RefPtr<Gdk::Pixbuf> scale_pixbuf(
        const Glib::RefPtr<Gdk::Pixbuf>& original_pixbuf,
        double current_scale,
        double scale_factor,
        double& updated_scale);
};

#endif