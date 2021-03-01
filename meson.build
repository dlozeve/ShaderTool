project(
  'shadertool', 'c',
  version: '0.1',
  default_options: [
    'buildtype=debugoptimized',
    'warning_level=3',
    'werror=true',
  ]
)

cc = meson.get_compiler('c')

glfw_dep = dependency('glfw3')
glew_dep = dependency('glew')
freeimage_dep = cc.find_library('freeimage')

executable(
  'shadertool',
  sources: ['src/main.c', 'src/renderer.c', 'src/shaders.c', 'src/io.c', 'src/log.c'],
  dependencies: [glfw_dep, glew_dep, freeimage_dep],
  c_args: '-DLOG_USE_COLOR',
)
