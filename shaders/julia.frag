#version 330 core

out vec4 fragColor;

uniform float u_time;
uniform uint u_frame;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

// Source:
// https://github.com/kbinani/colormap-shaders/blob/master/shaders/glsl/MATLAB_jet.frag
float colormap_red(float x) {
  if (x < 0.7) {
    return 4.0 * x - 1.5;
  } else {
    return -4.0 * x + 4.5;
  }
}

float colormap_green(float x) {
  if (x < 0.5) {
    return 4.0 * x - 0.5;
  } else {
    return -4.0 * x + 3.5;
  }
}

float colormap_blue(float x) {
  if (x < 0.3) {
    return 4.0 * x + 0.5;
  } else {
    return -4.0 * x + 2.5;
  }
}

vec4 colormap(float x) {
  float r = clamp(colormap_red(x), 0.0, 1.0);
  float g = clamp(colormap_green(x), 0.0, 1.0);
  float b = clamp(colormap_blue(x), 0.0, 1.0);
  return vec4(r, g, b, 1.0);
}

// Julia
vec2 f(vec2 z, vec2 c) {
  return vec2(z.x * z.x - z.y * z.y, 2 * z.x * z.y) + c;
}

float pi = 2.0 * asin(1.0);

void main() {
  // Changes with time
  // float a = 2.0 * pi * (int(u_frame) % 500) / 500.0;
  // vec2 c = 0.7885 * vec2(cos(a), sin(a));

  // Changes with cursor position
  vec2 c = u_mouse.xy / u_resolution.xy * 2.0 - vec2(1.0, 1.0);

  vec2 z = gl_FragCoord.xy / u_resolution.xy * 3.0 - vec2(1.5, 1.5);
  int i = 0;
  while (length(z) <= 2 && i < 1000) {
    z = f(z, c);
    i++;
  }
  if (i == 1000) {
    fragColor = vec4(vec3(0.0), 1.0);
  } else {
    fragColor = colormap(smoothstep(0.0, 20.0, i));
  }
}
