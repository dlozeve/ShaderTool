project(
  'shadertool', 'c',
  version: '0.1',
  default_options: [
    'buildtype=debugoptimized',
    'warning_level=3',
    'c_std=c99',
    'werror=true',
  ]
)

glfw_dep = dependency('glfw3')
glew_dep = dependency('glew')

executable(
  'shadertool',
  sources: ['main.c', 'renderer.c', 'shaders.c', 'log.c'],
  dependencies: [glfw_dep, glew_dep],
  c_args: '-DLOG_USE_COLOR',
)