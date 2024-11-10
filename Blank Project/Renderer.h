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

	SceneNode* root;
	HeightMap* heightMap;
	Shader* landscapeShader;
	Shader* nodeShader;
	Camera* camera;
	GLuint mountainTex;
	GLuint valleyTex;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
