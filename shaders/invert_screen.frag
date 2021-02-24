#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float u_time;
uniform uint u_frame;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform sampler2D u_texture;

void main() {
  //FragColor = texture(u_texture, TexCoords);
  FragColor = vec4(1.0 - texelFetch(u_texture, ivec2(gl_FragCoord), 0));
}
