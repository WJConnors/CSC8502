#pragma once
#include "../NCLGL/OGLRenderer.h"
class HeightMap;
class Camera;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);
	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;
protected:
	HeightMap* heightMap;
	Shader* shader;
	Camera* camera;
	GLuint mountainTex;
	GLuint valleyTex;
};
