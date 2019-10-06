#version 150

in  vec4 vPosition;
uniform vec4 OurColor;
out vec4 color;

void main() 
{
  gl_Position = vPosition;
  color = OurColor;
} 
