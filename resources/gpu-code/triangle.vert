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
attribute vec2 positionA;
attribute vec2 positionB;
attribute vec2 positionC;
attribute vec3 colorA;
attribute vec3 colorB;
attribute vec3 colorC;
#else
in vec2 position;
in vec2 positionA;
in vec2 positionB;
in vec2 positionC;
in vec3 colorA;
in vec3 colorB;
in vec3 colorC;
#endif

out vec2 oPosition;
out vec2 oPositionA;
out vec2 oPositionB;
out vec2 oPositionC;
out vec3 oColorA;
out vec3 oColorB;
out vec3 oColorC;

void main()
{
  gl_Position.xy = position;
  gl_Position.z = 0.0;
  gl_Position.w = 1.0;
  oPosition = position;
  oPositionA = positionA;
  oPositionB = positionB;
  oPositionC = positionC;
  oColorA = colorA;
  oColorB = colorB;
  oColorC = colorC;
}
