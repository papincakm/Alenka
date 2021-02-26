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
attribute vec2 positions[6];
attribute vec3 colors[6];
attribute vec3 barCoords;
#else
in vec2 currentPosition;
in vec2 positions[6];
in vec3 colors[6];
in vec3 barCoords;
#endif

out vec2 oCurrentPosition;
flat out vec2 oPositions[6];
flat out vec3 oColors[6];
out vec3 oBarCoords;

void main()
{
  gl_Position.xy = currentPosition;
  gl_Position.z = 0.0;
  gl_Position.w = 1.0;
  oCurrentPosition = currentPosition;
  for (int i = 0; i < 6; i++) {
    oPositions[i] = positions[i];
    oColors[i] = colors[i];
  }
  oBarCoords = barCoords;
}
