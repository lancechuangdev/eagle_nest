project('eagle_nest', 'cpp')

# Core dependencies
gtkmm_dep = dependency('gtkmm-3.0', version: '>= 3.24.0')

# -------------------------------
# Resource Compilation
# -------------------------------

# Import gnome module for resource compilation
gnome = import('gnome')

# Generate the binary resource file, no need to install it directly
resource_target = gnome.compile_resources(
    'eagle_nest_resources',
    'eagle_nest_gresource.xml',
    source_dir: 'icons',  # Directory where your icons are located
)

# -------------------------------
# Executable
# -------------------------------

executable('en', 
    [resource_target, 'main.cpp', 'main_window.cpp'],
    dependencies: [gtkmm_dep]
)