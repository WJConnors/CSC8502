#version 330 core

uniform sampler2D mountainTex;
uniform sampler2D valleyTex;
uniform float heightThreshold;
uniform float transitionWidth;

uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

in Vertex {
    vec2 texCoord;
	float fragHeight;
	vec4 colour;
	vec3 normal;
	vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void) {
    vec4 mountainColor = texture(mountainTex, IN.texCoord);
    vec4 valleyColor = texture(valleyTex, IN.texCoord);

    // Calculate blendFactor between 0 and heightThreshold with a bias toward valleyColor
    float blendFactor = clamp(IN.fragHeight / heightThreshold, 0.0, 1.0);
    blendFactor = pow(blendFactor, 0.8); // Apply bias with an exponent (0.5 for strong bias)

    // Blend the two textures based on the calculated blendFactor
    vec4 blendedColour = mix(valleyColor, mountainColor, blendFactor);

    vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

    //vec4 diffuse = texture(blendedColour, IN.texCoord);

	float lambert = max(dot(incident, IN.normal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, IN.normal),0.0,1.0);
	specFactor = pow(specFactor, 60.0);

	vec3 surface = (blendedColour.rbg * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = blendedColour.a;
}