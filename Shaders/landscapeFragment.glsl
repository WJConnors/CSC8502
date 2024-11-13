#version 330 core

uniform sampler2D mountainTex;
uniform sampler2D valleyTex;
uniform sampler2D mountainBump;
uniform sampler2D valleyBump;
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
	vec3 tangent;
	vec3 binormal;
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

	vec3 mountainNormal = texture(mountainBump, IN.texCoord).rgb;
    vec3 valleyNormal = texture(valleyBump, IN.texCoord).rgb;

	vec3 blendedNormal = mix(valleyNormal, mountainNormal, blendFactor);

    vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	mat3 TBN = mat3(normalize(IN.tangent),
                    normalize(IN.binormal), 
                    normalize(IN.normal));

    //vec4 diffuse = texture(blendedColour, IN.texCoord);
	vec3 bumpNormal = normalize(TBN * normalize(blendedNormal * 2.0 - 1.0));

	float lambert = max(dot(incident, bumpNormal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, bumpNormal),0.0,1.0);
	specFactor = pow(specFactor, 60.0);

	vec3 surface = (blendedColour.rbg * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = blendedColour.a;
}