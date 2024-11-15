#version 330 core
uniform sampler2D diffuseTex;
uniform float transitionTime;

in Vertex {
	vec2 texCoord;
} IN;

float rand(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

out vec4 fragColour;

void main(void) {
	// Map transitionTime to a percentage for randomness
	float transitionPercent;
	if (transitionTime <= 2.5) {
		// Increase from 0% to 100% over the first half
		transitionPercent = transitionTime / 2.5;
	} else {
		// Decrease from 100% to 0% over the second half
		transitionPercent = 1.0 - (transitionTime - 2.5) / 2.5;
	}

	vec2 seed = gl_FragCoord.xy;
	float randomValue = rand(seed);

	// Base texture color
	fragColour = texture(diffuseTex, IN.texCoord);

	// Replace with random colors based on randomness threshold
	if (randomValue < transitionPercent) {
		float random1 = rand(seed + vec2(1.0, 0.0)); // Slight offset
		float random2 = rand(seed + vec2(0.0, 1.0)); // Different offset
		float random3 = rand(seed + vec2(1.0, 1.0)); // Combined offset
		fragColour = vec4(random1, random2, random3, 1.0);
	}
}
