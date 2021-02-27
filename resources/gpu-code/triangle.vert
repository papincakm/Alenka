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
attribute vec2 currentPosition;
attribute float frequency;
#else
in vec2 currentPosition;
in float frequency;
#endif
out float oFrequency;

void main()
{
  gl_Position.xy = currentPosition;
  gl_Position.z = 0.0;
  gl_Position.w = 1.0;
  oFrequency = frequency;
}
