#version 330 core

uniform sampler2D mountainTex;
uniform sampler2D valleyTex;
uniform sampler2D mountainBump;
uniform sampler2D valleyBump;
uniform float heightThreshold;
uniform float transitionWidth;

in Vertex {
	vec2 texCoord;
	float fragHeight;
	vec4 colour;
	vec3 normal;
	vec3 worldPos;
	vec3 tangent;
	vec3 binormal;
} IN;

out vec4 fragColour[2];

void main(void) {
	vec4 mountainColor = texture(mountainTex, IN.texCoord);
    vec4 valleyColor = texture(valleyTex, IN.texCoord);

    // Calculate blendFactor between 0 and heightThreshold with a bias toward valleyColor
    float blendFactor = clamp(IN.fragHeight / heightThreshold, 0.0, 1.0);
    blendFactor = pow(blendFactor, 0.8); // Apply bias with an exponent (0.5 for strong bias)

    // Blend the two textures based on the calculated blendFactor
    vec4 blendedColour = mix(valleyColor, mountainColor, blendFactor);

	vec3 mountainNormal = texture(mountainBump, IN.texCoord).rgb;
    vec3 valleyNormal = texture(valleyBump, IN.texCoord).rgb;

	vec3 blendedNormal = mix(valleyNormal, mountainNormal, blendFactor);

	mat3 TBN = mat3(normalize(IN.tangent),
                    normalize(IN.binormal),
                    normalize(IN.normal));

	vec3 worldNormal = normalize(TBN * (blendedNormal * 2.0 - 1.0));

	fragColour[0] = blendedColour;
	fragColour[1] = vec4(worldNormal * 0.5 + 0.5, 1.0);
}