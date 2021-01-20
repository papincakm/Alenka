/**
 * @brief Source code of fragment shader used to draw a circle.
 *
 * @file
 * @include single.vert
 */
/// @cond

precision highp float;

#ifdef GLSL_110
#extension GL_EXT_gpu_shader4 : enable
attribute vec2  pointPos;
#else
in vec2  pointPos;
#endif

uniform float aRadius;
uniform vec4  aColor;
uniform vec2 u_resolution;

#ifndef GLSL_110
out vec4  outColor;
#endif

const float threshold = 0.3;

void main()
{
	vec2 st = gl_PointCoord.xy;

    float dist = distance(pointPos, st - 0.5);

	//dont touch pixels beyond radius of the circle
    if (dist > 0.5)
    	discard;

	float accelerator = 2;
		
	vec3 color = vec3(0.6, 0.6, 0.6);

#ifdef GLSL_110
  	gl_FragColor = vec4(color, 1.0);
#else
  	outColor = vec4(color, 1.0);
#endif
}
/// @endcond
