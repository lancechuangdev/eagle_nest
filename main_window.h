#ifndef EAGLE_EYE_MAIN_WINDOW_H
#define EAGLE_EYE_MAIN_WINDOW_H

#include <gtkmm.h>

class MainWindow : public Gtk::Window
{
public:
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
    Gtk::Button *m_dataset_sources_refresh_btn;
    Gtk::ListBox *m_dataset_sources_lbox;
    Gtk::Grid *m_dataset_sources_grid;
    Gtk::CheckButton *m_select_all_datasources_cbtn;

    void on_previous_clicked();
    void on_next_clicked();
    void on_dataset_sources_refresh_clicked();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    int m_current_step = 0;
    std::vector<std::string> m_training_page_names = {"page_select_model", "page_select_images", "page_training"};
    std::vector<Gtk::Label*> m_training_step_labels;

    void set_window_title(const std::string &title);
    void on_menu_toggled();
    void update_step_indicator();
    void transition_step(bool step_forward);
    void write_model_readme();
    void refresh_dataset_sources();
    void remove_dataset_sources_except_header();
    void add_dataset_sources_header();
    void add_dataset_source_row(const std::string& name, const std::string& type, const std::string& connection_info, bool checked = true, const std::string& connection_status = "Unknown");
    void update_all_datasource_connection_status();
    std::string get_connection_status(const std::string& connection_info);
};

#endif