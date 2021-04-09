/**
 * @brief Source code of the fragment shader used for all the drawing modes.
 *
 * @file
 * @include color.frag
 */
/// @cond

uniform sampler1D colormap;

#ifndef GLSL_110
out vec4  outColor;
#endif

#ifdef GLSL_110
#extension GL_EXT_gpu_shader4 : enable
attribute vec3 oBarCoords;
#else
in float oFrequency;
#endif


#ifdef GLSL_110
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
#endif

void main() {

	
#ifdef GLSL_110
	const float bins = 70.0f;	
	for (float i = 0; i < bins; i++) {
		float f = i / bins;

		if (f > oFrequency) {
			color = vec4(getColorGrad(f), 1);
			break;
		}
	}
#else
	vec4 color = texture(colormap, oFrequency).rgba;
#endif
#ifdef GLSL_110
    gl_FragColor = color;
#else
    outColor = color;
#endif
}
/// @endcond
