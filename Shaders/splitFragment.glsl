#version 330 core

uniform sampler2D diffuseTex; // The texture sampler
uniform bool output;          // Whether to draw the left or right side
uniform float width;            // The dividing width

in Vertex {
    vec2 texCoord;            // Input texture coordinates
} IN;

out vec4 fragColour;          // Output fragment color

void main(void) {
    // Get the screen-space X coordinate for this fragment
    float screenX = gl_FragCoord.x;

    // Discard fragments based on the value of `output` and `width`
    if (output) {
        // Render only the fragments to the right of the width
        if (screenX <= width) {
            discard; // Skip these fragments
        }
    } else {
        // Render only the fragments to the left of the width
        if (screenX > width) {
            discard; // Skip these fragments
        }
    }

    // Sample the texture and assign the color
    fragColour = texture(diffuseTex, IN.texCoord);
}
