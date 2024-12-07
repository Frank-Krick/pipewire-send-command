project('pipewire-send-command', 'cpp', default_options : ['cpp_std=c++23'])
pipewire_dep = dependency('libpipewire-0.3')

include_directory = include_directories('include')

executable(
  'pipewire-send-command',
  'src/main.cpp',
  dependencies: [pipewire_dep],
  include_directories: [include_directory])
