/**
 * @brief Source code of vertex shader used to draw a triangle.
 *
 * @file
 */
/// @cond

precision mediump float;

//TODO: find out how does multiple attributes in GLSL110 work
#ifdef GLSL_110
#extension GL_EXT_gpu_shader4 : enable
attribute vec2 position;
attribute float frequency;
//attribute vec3 color;
#else
in vec2 position;
in float frequency;
in vec3 color;
#endif

out float oFrequency;
out vec2 oPosition;
//out vec3 oColor;

void main()
{
  gl_Position.xy = position;
  gl_Position.z = 0.0;
  gl_Position.w = 1.0;
  oFrequency = frequency;
  oPosition = position;
  //oColor = color;
}
