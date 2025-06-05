#include "expand_collapse_btn.h"
#include <gdkmm/pixbuf.h>
#include <gtkmm/image.h>
#include <gtkmm/cssprovider.h>

#include <iostream>
ExpandCollapseButton::ExpandCollapseButton(bool initial_expanded)
    : is_expanded(initial_expanded)
{
    // Load CSS once and apply
    static auto css_provider = []() {
        auto provider = Gtk::CssProvider::create();
        provider->load_from_data(R"(
            .borderless {
                border: none;
                box-shadow: none;
                background: none;
                padding: 0;
                min-width: 0;
                min-height: 0;
            }
            .borderless:hover,
            .borderless:active {
                background: none;
                box-shadow: none;
            }
        )");

        Gtk::StyleContext::add_provider_for_screen(
            Gdk::Screen::get_default(),
            provider,
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        return provider;
    }();

    // Set the button to be borderless
    get_style_context()->add_class("borderless");

    update_icon(); // Load initial icon
    set_expanded(initial_expanded);  // Initial state: collapsed
    signal_clicked().connect(sigc::mem_fun(*this, &ExpandCollapseButton::on_button_clicked));
}

void ExpandCollapseButton::set_expanded(bool expanded)
{
    if (is_expanded != expanded) {
        is_expanded = expanded;
        update_icon();
    }
}

bool ExpandCollapseButton::get_expanded() const
{
    return is_expanded;
}

void ExpandCollapseButton::on_button_clicked()
{
    is_expanded = !is_expanded;
    update_icon();
    signal_toggled.emit(is_expanded);
}

void ExpandCollapseButton::update_icon()
{
    const char* resource_path = is_expanded
        ? "/com/example/eagle_nest/collapse.svg"
        : "/com/example/eagle_nest/expand.svg";

    try {
        auto pixbuf = Gdk::Pixbuf::create_from_resource(resource_path);
        if (pixbuf) {
            auto image = Gtk::make_managed<Gtk::Image>(pixbuf);
            set_image(*image);
        } else {
            std::cerr << "Failed to load icon from " << resource_path << std::endl;
        }
    } catch (const Glib::Error& ex) {
        std::cerr << "Error loading resource: " << ex.what() << std::endl;
    }
}