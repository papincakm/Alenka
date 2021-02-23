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
attribute vec3 oBarPosition;
attribute vec3 freqVec;
attribute vec3 oColorA;
attribute vec3 oColorB;
attribute vec3 oColorC;
#else
//mby change this to frequencyRatioToMax or smthng like that
in vec2 oPosition;
in vec2 oPositionA;
in vec2 oPositionB;
in vec2 oPositionC;
in vec3 oBarPosition;
in vec3 freqVec;
in vec3 oColorA;
in vec3 oColorB;
in vec3 oColorC;
#endif

uniform vec2 u_resolution;

vec3 getColor(float oFrequency) {
	const float firstT = 0.25f;
	const float secondT = 0.5f;
	const float thirdT = 0.75f;

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

	return vec3(red, green, blue);
}

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

	vec4 color;// = vec4(oColorA, 1.0);

	//vec4 color = vec4((oBarPosition.x * oColorA) + (oBarPosition.y * oColorB) + (oBarPosition.z * oColorC), 1.0);

	float mult = 1.5;

	color = vec4(calcColor(), 1.0);

	/*if (oBarPosition.x > 0.5) {
		//color = vec4((2.0f - oBarPosition.x) * oColor.x, oColor.y, oColor.z, 1.0);
		//color = vec4(((2.0f - oBarPosition.x) * oColorA) + (oBarPosition.y * oColorB) + (oBarPosition.z * oColorC), 1.0);
	}
	else if (oBarPosition.y > 0.5) {
		//color = vec4(oColor.x, (2.0f - oBarPosition.y) * oColor.y, oColor.z, 1.0);
		color = vec4((oBarPosition.x * oColorA) + ((2.0f - oBarPosition.y) * oColorB) + (oBarPosition.z * oColorC), 1.0);
	} 
	else if (oBarPosition.z > 0.5) {
		//color = vec4(oColor.x, oColor.y, (2.0f - oBarPosition.z) * oColor.z, 1.0);
		color = vec4((oBarPosition.x * oColorA) + (oBarPosition.y * oColorB) + ((2.0f - oBarPosition.z) * oColorC), 1.0);
	}
	else {
		color = vec4((oBarPosition.x * oColorA) + (oBarPosition.y * oColorB) + (oBarPosition.z * oColorC), 1.0);
	}*/

	/*if (oBarPosition.x > 0.66) {
		color = vec4((2.0f - oBarPosition.x) * oColor.x, oColor.y, oColor.z, 1.0);
	}
	else if (oBarPosition.y > 0.66) {
		color = vec4(oColor.x, (2.0f - oBarPosition.y) * oColor.y, oColor.z, 1.0);
	} 
	else if (oBarPosition.z > 0.66) {
		color = vec4(oColor.x, oColor.y, (2.0f - oBarPosition.z) * oColor.z, 1.0);
	}
	else {
		color = vec4(oColor, 1.0);
	}*/		

	//vec4 color = vec4(step(vec2(0.45), st), 0, 1);

	//vec4 color = vec4(int((oPosition.x * 100)) % 2, int((oPosition.x * 100)) % 3, 1, 1);


#ifdef GLSL_110
    //gl_FragColor = vec4(oColor, 1.0);
    gl_FragColor = color;
#else
    //outColor = vec4(oColor, 1.0);
    outColor = color;
#endif
}
/// @endcond
