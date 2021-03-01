/**
 * @brief Source code of vertex shader used to draw a single vertext point.
 *
 * @file
 */
/// @cond

precision mediump float;

#ifdef GLSL_110
#extension GL_EXT_gpu_shader4 : enable
attribute vec4 vPosition;
#else
in vec4   vPosition;
#endif

out       vec2 pointPos;

void main()
{
    gl_Position  = vPosition;
    gl_PointSize = 10f;
}
