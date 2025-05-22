#ifndef EAGLE_EYE_EXPAND_COLLAPSE_H
#define EAGLE_EYE_EXPAND_COLLAPSE_H

#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <sigc++/signal.h>

class ExpandCollapseButton : public Gtk::Button {
public:
    ExpandCollapseButton();

    // External access
    void set_expanded(bool expanded);
    bool get_expanded() const;

    // Custom signal: emitted when toggled
    sigc::signal<void(bool)> signal_toggled;

private:
    bool is_expanded = false;
    
    void on_button_clicked();
    void update_icon();
};

#endif