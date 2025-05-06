#version 330 core

uniform sampler2D sceneTex;
uniform int isVertical;  // 1 for vertical blur, 0 for horizontal blur

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;

// Predefined scale factors for the Gaussian blur kernel
const float scaleFactors[7] = float [](0.006 , 0.061 , 0.242 , 0.383 , 0.242 , 0.061 , 0.006);

void main(void) {
	fragColor = vec4(0, 0, 0, 1);
	vec2 delta = vec2(0, 0);

	// Determine if the blur is vertical or horizontal and calculate appropriate offset
	if(isVertical == 1) {
		delta = dFdy(IN.texCoord);
	}
	else {
		delta = dFdx(IN.texCoord);
	}
	// Loop through and apply the blur
	for(int i = 0; i < 7; i++) {
		vec2 offset = delta * (i - 3);
		vec4 tmp = texture2D(sceneTex, IN.texCoord.xy + offset);
		fragColor += tmp * scaleFactors[i];
	}
}
