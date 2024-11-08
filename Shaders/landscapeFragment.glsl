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

	float blendFactor = smoothstep(heightThreshold - transitionWidth, heightThreshold + transitionWidth, IN.fragHeight);
    fragColour = mix(valleyColor, mountainColor, blendFactor);
}