/**
 * @brief Source code of fragment shader used to draw a circle.
 *
 * @file
 * @include single.vert
 */
/// @cond

#ifdef GLSL_110
#extension GL_EXT_gpu_shader4 : enable
attribute vec2 pointPos;
#else
in vec2 pointPos;
#endif

uniform vec2 u_resolution;

float circle(in vec2 _st, in float _radius){
    vec2 dist = _st;
	return 1.-smoothstep(_radius-(_radius*0.01),
                         _radius+(_radius*0.01),
                         dot(dist,dist)*4.0);
}

void main(){
	vec2 st = gl_FragCoord.xy/u_resolution.xy;

	vec3 color = vec3(circle(st - vec2(0.5),0.1));

	gl_FragColor = vec4( color, 1.0 );
}
/// @endcond
