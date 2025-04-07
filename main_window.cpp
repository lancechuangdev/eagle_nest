#include "main_window.h"

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

    update_step_indicator();
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
}


void MainWindow::update_step_indicator() {
    for (size_t i = 0; i < m_training_step_labels.size(); ++i) {
        auto current_step_name = m_training_step_labels[i]->get_text();
        m_training_step_labels[i]->set_markup(i == m_current_step ? "<b><span foreground='blue'>" + current_step_name + "</span></b>" : current_step_name);
    }
}