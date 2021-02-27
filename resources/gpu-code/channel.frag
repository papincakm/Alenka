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
attribute vec2 oCurrentPosition;
attribute vec2 oPositions[6];
attribute vec3 oColors[6];
attribute vec3 oBarCoords;
#else
in vec2 oCurrentPosition;
flat in vec2 oPositions[6];
flat in vec3 oColors[6];
in vec3 oBarCoords;
in float oFrequency;
#endif

vec3 calcColor() {
	const int vecCount = 6;

	float total = 0;
	float red = 0;
	float green = 0;
	float blue = 0;
	
	float distances[6];
	
	vec2 curPos = vec2(oBarCoords.x * oPositions[0] + oBarCoords.y * oPositions[1] + oBarCoords.z * oPositions[2]);

	for (int i = 0; i < 6; i++) {
		if (oPositions[i] == -1)
			continue;

		distances[i] = distance(curPos, oPositions[i]);
		if (distances[i] == 0)
			return oColors[i];

		distances[i] = 1 / (distances[i] * distances[i]);
		total += distances[i];
	}

	for (int i = 0; i < vecCount; i++) {
		if (oPositions[i] == -1)
			continue;
			
		red += distances[i] / total * oColors[i].x;
		green += distances[i] / total * oColors[i].y;
		blue += distances[i] / total * oColors[i].z;
	}

	return vec3(red, green, blue);
}

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

	vec4 color;// = vec4(calcColor(), 1.0);
	
	//float freqColor = 255 * oFrequency;
	
	const float bins = 50.0f;	

	for (float i = 0; i < bins; i++) {
		float f = i / bins;
		if (f > oFrequency) {
			//color = vec4(f + 0.04 * oFrequency, 0, 0, 1);
			color = vec4(getColorGrad(f + 0.04 * oFrequency), 1);
			break;
		}
	}
	//vec4 color = vec4(oBarCoords.x * oColors[0] + oBarCoords.y * oColors[1] + oBarCoords.z * oColors[2], 1.0);

#ifdef GLSL_110
    gl_FragColor = color;
#else
    outColor = color;
#endif
}
/// @endcond
