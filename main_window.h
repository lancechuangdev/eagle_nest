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

    void on_previous_clicked();
    void on_next_clicked();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    int m_current_step = 0;
    std::vector<std::string> m_training_page_names = {"page_select_model", "page_select_images", "page_training"};
    std::vector<Gtk::Label*> m_training_step_labels;

    void set_window_title(const std::string &title);
    void on_menu_toggled();
    void update_step_indicator();
};

#endif