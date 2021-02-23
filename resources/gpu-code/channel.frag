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
attribute vec2 oPosition;
attribute vec2 oPositionA;
attribute vec2 oPositionB;
attribute vec2 oPositionC;
attribute vec3 oColorA;
attribute vec3 oColorB;
attribute vec3 oColorC;
#else
//mby change this to frequencyRatioToMax or smthng like that
in vec2 oPosition;
in vec2 oPositionA;
in vec2 oPositionB;
in vec2 oPositionC;
in vec3 oColorA;
in vec3 oColorB;
in vec3 oColorC;
#endif

uniform vec2 u_resolution;

vec3 calcColor() {
	float total = 0;
	float red = 0;
	float green = 0;
	float blue = 0;

	float distA = distance(oPosition, oPositionA);
	float distB = distance(oPosition, oPositionB);
	float distC = distance(oPosition, oPositionC);

	distA = 1 / (distA * distA);
	distB = 1 / (distB * distB);
	distC = 1 / (distC * distC);

	total = distA + distB + distC;

	red = distA / total * oColorA.x + distB / total * oColorB.x + distC / total * oColorC.x;
	green = distA / total * oColorA.y + distB / total * oColorB.y + distC / total * oColorC.y;
	blue = distA / total * oColorA.z + distB / total * oColorB.z + distC / total * oColorC.z;

	return vec3(red, green, blue);
}

void main() {
	vec2 st = gl_FragCoord.xy / u_resolution;

	vec4 color;

	float mult = 1.5;

	color = vec4(calcColor(), 1.0);

#ifdef GLSL_110
    gl_FragColor = color;
#else
    outColor = color;
#endif
}
/// @endcond
