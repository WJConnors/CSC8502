#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D diffuseLight;
uniform sampler2D specularLight;

in Vertex {
    vec2 texCoord;
} IN;

out vec4 fragColour;

void main(void) {
    vec3 diffuse = texture(diffuseTex, IN.texCoord).rgb;
    vec3 light = texture(diffuseLight, IN.texCoord).rgb;
    vec3 specular = texture(specularLight, IN.texCoord).rgb;

    fragColour.rgb = diffuse * 0.1; // Ambient light
    fragColour.rgb += diffuse * light; // Diffuse lighting
    fragColour.rgb += specular; // Specular lighting
    fragColour.a = 1.0;
}
