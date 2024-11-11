#pragma once
#include "../NCLGL/OGLRenderer.h"
#include "../nclgl/Frustum.h"
class HeightMap;
class Camera;
class SceneNode;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);
	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;
protected:
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* n);
	void DrawHeightMap();

	void DrawScene();
	void PresentScene();
	void GenBuffers();

	SceneNode* root;
	Mesh* quad;
	HeightMap* heightMap;
	Shader* landscapeShader;
	Shader* nodeShader;
	Shader* sceneShader;
	Camera* camera;
	GLuint mountainTex;
	GLuint valleyTex;
	GLuint bufferFBO;
	GLuint bufferColourTex;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
