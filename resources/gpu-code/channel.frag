/**
 * @brief Source code of the fragment shader used for all the drawing modes.
 *
 * @file
 * @include color.frag
 */
/// @cond

#ifndef GLSL_110
out vec4  outColor;
#endif

#ifdef GLSL_110
#extension GL_EXT_gpu_shader4 : enable
attribute vec3 inColor;
#else
in vec3 vColor;
#endif

uniform vec2 u_resolution;

void main() {
	vec2 st = gl_FragCoord.xy / u_resolution;

	vec4 color = vec4(step(vec2(0.45), st), 0, 1);

#ifdef GLSL_110
    gl_FragColor = vec4(vColor, 1.0);
    //gl_FragColor = color;
#else
    outColor = vec4(vColor, 1.0);
    //outColor = color;
#endif
}
/// @endcond
