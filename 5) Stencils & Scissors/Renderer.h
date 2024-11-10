#pragma once

#include "../nclgl/OGLRenderer.h"

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);

	void RenderScene() override;

	void ToggleScissor();
	void ToggleStencil();

protected:
	Shader* landscapeShader;
	Mesh* meshes[2];
	GLuint textures[2];

	bool usingScissor;
	bool usingStencil;
};