#version 330 core

uniform sampler2D mountainTex;
uniform sampler2D valleyTex;
uniform float heightThreshold;
uniform float transitionWidth;

in Vertex {
	vec2 texCoord;
	float fragHeight;
} IN;

out vec4 fragColour;

void main(void) {
    vec4 mountainColor = texture(mountainTex, IN.texCoord);
    vec4 valleyColor = texture(valleyTex, IN.texCoord);

    // Calculate blendFactor between 0 and heightThreshold with a bias toward valleyColor
    float blendFactor = clamp(IN.fragHeight / heightThreshold, 0.0, 1.0);
    blendFactor = pow(blendFactor, 0.8); // Apply bias with an exponent (0.5 for strong bias)

    // Blend the two textures based on the calculated blendFactor
    fragColour = mix(valleyColor, mountainColor, blendFactor);
}