#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D diffuseLight;
uniform sampler2D specularLight;
uniform sampler2D depthTex;

in Vertex {
    vec2 texCoord;
} IN;

out vec4 fragColour;

void main(void) {
	float depth = texture(depthTex, IN.texCoord).r;
	if(depth == 1.0) {
		discard;
	}

    vec3 diffuse = texture(diffuseTex, IN.texCoord).rgb;
    vec3 light = texture(diffuseLight, IN.texCoord).rgb;
    vec3 specular = texture(specularLight, IN.texCoord).rgb;

    fragColour.rgb = diffuse * 0.1; // Ambient light
    fragColour.rgb += diffuse * light; // Diffuse lighting
    fragColour.rgb += specular; // Specular lighting
    fragColour.a = 1.0;
}
