project('eagle_nest', 'cpp')

# Core dependencies
gtkmm_dep = dependency('gtkmm-3.0', version: '>= 3.24.0', required: true)
json_dep = dependency('nlohmann_json', required: true)
openssl_dep = dependency('openssl') # install OpenSSLfirst: sudo apt install libssl-dev

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
    [resource_target, 'main.cpp', 'main_window.cpp', 'app_paths.cpp'],
    dependencies: [gtkmm_dep, openssl_dep]
)