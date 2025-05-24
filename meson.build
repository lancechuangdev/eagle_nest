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
    [resource_target, 'main.cpp', 'main_window.cpp', 'expand_collapse_btn.cpp', 'app_paths.cpp'],
    install: true,
    install_dir: join_paths(get_option('prefix'), 'bin', 'eagle_nest'),
    dependencies: [gtkmm_dep, openssl_dep]
)

# -------------------------------
# Installations
# -------------------------------

# Install application data files
install_data(
    ['ui.glade', 'train_model.py', 'test_model.py', 'convert_model.py'],
    install_dir: join_paths(get_option('prefix'), 'share', 'eagle_nest')
)

# Install app icon to the system icons directory
install_data(
    'com.example.eagle_nest.png',
    install_dir: '/usr/share/icons/hicolor/512x512/apps',
)

# Install .desktop file to system-wide applications directory
install_data(
    'eagle_nest.desktop',
    install_dir: '/usr/share/applications'
)