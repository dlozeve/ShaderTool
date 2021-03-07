# ShaderTool

[![build](https://github.com/dlozeve/ShaderTool/actions/workflows/build.yml/badge.svg)](https://github.com/dlozeve/ShaderTool/actions/workflows/build.yml)
[![docs](https://github.com/dlozeve/ShaderTool/actions/workflows/docs.yml/badge.svg)](https://dlozeve.github.io/ShaderTool/)

Live tool for developing OpenGL shaders interactively.

I developed this small program to experiment with shaders locally, to
reproduce an experience like [Shadertoy](https://www.shadertoy.com/)
offline, with the ability to choose my own text editor. It was also a
good project to learn OpenGL development. For this reason, the code is
very minimal and should be fairly readable.

Additional features:

- Extensive logging (using the nice
  [log.c](https://github.com/rxi/log.c) library)
- FPS tracking
- Reload shaders automatically on save (using
  [inotify](https://man.archlinux.org/man/inotify.7))
- Save screenshot to a file
- Complete argument parsing with
  [Argp](https://www.gnu.org/software/libc/manual/html_node/Argp.html)
- Full documentation with [Doxygen](https://www.doxygen.nl/index.html)

## Build

This project requires the [GLFW](https://www.glfw.org/),
[GLEW](http://glew.sourceforge.net/), and
[FreeImage](https://freeimage.sourceforge.io/) libraries. On a
Debian/Ubuntu system:
```sh
sudo apt-get install libglfw3-dev libglew-dev libfreeimage-dev
```

To build (with [Meson](https://mesonbuild.com/)):
```sh
meson build
ninja -C build
```

To build the documentation with
[Doxygen](https://www.doxygen.nl/index.html):
```sh
doxygen Doxyfile
```

The documentation is also available
[online](https://dlozeve.github.io/ShaderTool/).

## Usage

```
Usage: shadertool [OPTION...] SHADER
                                    Compile and render the SHADER.
ShaderTool -- Live tool for developing OpenGL shaders interactively

  -b, --buffer=FILE          Source file of the buffer fragment shader
  -r, --auto-reload          Automatically reload on save
  -s, -q, --silent, --quiet  Don't produce any output
  -v, --verbose              Produce verbose output
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

For instance, to run the [mandelbrot](shaders/mandelbrot.frag) shader
with live reloading on save:
```sh
shadertool -r shaders/mandelbrot.frag
```

Keyboard shortcuts:

- `Escape` to quit
- `R` to reload the shaders
- `S` to save a screenshot to the current directory, in a file
  `shadername_frame_date_time.png`

## Limitations

For now, the "buffer" shader (i.e. the additional shader that renders
in a texture in another framebuffer) does not work properly. I don't
understand exactly what's broken, but maybe I'll investigate it more
closely later. If you have any idea about what went wrong, don't
hesitate to notify me!
