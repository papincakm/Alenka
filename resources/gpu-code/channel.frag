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
attribute vec3 oBarCoords;
#else
in float oFrequency;
#endif

vec3 getColorGrad(float intensity) {
		const float firstT = 0.25;
		const float secondT = 0.5;
		const float thirdT = 0.75;

		float red = 0;
		float green = 0;
		float blue = 0;

		if (intensity < firstT) {
			red = 0;
			green = intensity * 4;
			blue = 1;
		}
		else if (intensity < secondT) {
			red = 0;
			green = 1;
			blue = 1 - ((intensity - firstT) * 4);
		}
		else if (intensity < thirdT) {
			red = ((intensity - secondT) * 4);
			green = 1;
			blue = 0;
		}
		else {
			red = 1;
			green = 1 - ((intensity - thirdT) * 4);
			blue = 0;
		}

	return vec3(red, green, blue);
}

void main() {
	vec4 color;
	
	const float bins = 50.0f;	

	for (float i = 0; i < bins; i++) {
		float f = i / bins;
		if (f > oFrequency) {
			color = vec4(getColorGrad(f + 0.04 * oFrequency), 1);
			break;
		}
	}

#ifdef GLSL_110
    gl_FragColor = color;
#else
    outColor = color;
#endif
}
/// @endcond
