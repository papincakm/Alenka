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
attribute float oFrequency;
attribute vec2 oPosition;
#else
//mby change this to frequencyRatioToMax or smthng like that
in float oFrequency;
in vec2 oPosition;
#endif

uniform vec2 u_resolution;

void main() {
	vec2 st = gl_FragCoord.xy / u_resolution;

	//vec4 color = vec4(step(vec2(0.45), st), 0, 1);
	
	// thresholds
	const float firstT = 0.25;
	const float secondT = 0.5;
	const float thirdT = 0.75;
		
	float red = 0;
	float green = 0;
	float blue = 0;

	if (oFrequency < firstT) {
		red = 0;
		green = oFrequency * 4;
		blue = 1;
	}
	else if (oFrequency < secondT) {
		red = 0;
		green = 1;
		blue = 1 - ((oFrequency - firstT) * 4); 
	}
	else if (oFrequency < thirdT) {
		red = ((oFrequency - secondT) * 4);
		green = 1;
		blue = 0;
	}
	else {
		red = 1;
		green = 1 - ((oFrequency - thirdT) * 4);
		blue = 0;
	}

	vec4 color = vec4(red, green, blue, 1);
	//vec4 color = vec4(int((oPosition.x * 100)) % 2, int((oPosition.x * 100)) % 3, 1, 1);


#ifdef GLSL_110
    //gl_FragColor = vec4(color, 1.0);
    gl_FragColor = color;
#else
    //outColor = vec4(color, 1.0);
    outColor = color;
#endif
}
/// @endcond
