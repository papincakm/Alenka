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
attribute vec3 inColor;
#else
in vec2 position;
in vec3 inColor;
#endif

out vec3 vColor;

void main()
{
  gl_Position.xy = position;
  gl_Position.z = 0.0;
  gl_Position.w = 1.0;
  vColor = inColor;
}
