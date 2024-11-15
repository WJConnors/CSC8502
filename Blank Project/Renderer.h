#pragma once
#include "../NCLGL/OGLRenderer.h"
#include "../nclgl/Frustum.h"
#include "../nclgl/Camera.h"
class HeightMap;
class SceneNode;
class MeshAnimation;
class MeshMaterial;
class Light;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);
	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;
	 void SwitchScene() { winter = !winter; }
	 Vector3 GetCameraLocation() { return camera->GetPosition(); }
	 float GetCameraPitch() { return camera->GetPitch(); }
	 float GetCameraYaw() { return camera->GetYaw(); }
	 void EndCameraRail() { camEndedManually = true; }
protected:
	void DrawNode(SceneNode* n);
	void DrawHeightMap();
	void DrawWater();
	void DrawSkybox();
	
	void DrawScene();
	void DrawBlur();
	void PresentScene();
	void GenBuffers();

	void GenerateScreenTexture(GLuint& into, bool depth = false);
	void DrawPointLights();
	void CombineBuffers();
	GLuint gBufferFBO;
	GLuint gBufferColourTex;
	GLuint gBufferNormalTex;
	GLuint gBufferDepthTex;
	GLuint pointLightFBO;
	GLuint lightDiffuseTex;
	GLuint lightSpecularTex;
	Shader* gBufferShader;
	Shader* pointLightShader;
	Shader* combineShader;
	Light* pointLights;
	Mesh* sphere;

	SceneNode* root;
	Mesh* quad;
	HeightMap* heightMap;
	Shader* landscapeShader;
	Shader* nodeShader;
	Shader* sceneShader;
	Shader* processShader;
	Shader* reflectShader;
	Shader* skyboxShader;

	Camera* camera;
	GLuint mountainTex;
	GLuint mountainBump;
	GLuint valleyTex;
	GLuint valleyBump;
	GLuint bufferFBO;
	GLuint blurFBO;
	GLuint blurDepthTex;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex;

	GLuint cubeMap;
	GLuint waterTex;

	GLuint cubeMapSummer;
	GLuint valleyTexSummer;
	GLuint valleyBumpSummer;

	Frustum frameFrustum;

	vector<SceneNode*> nodeList;

	Mesh* dragonMesh;
	MeshAnimation* dragonAnim;
	MeshMaterial* dragonMaterial;
	vector<GLuint> dragonTextures;
	int curDragFrame;
	float dragFrameTime;
	Mesh* bearMesh;
	MeshAnimation* bearAnim;
	MeshMaterial* bearMaterial;
	vector<GLuint> bearTextures;
	int curBearFrame;
	float bearFrameTime;
	Shader* animShader;
	void DrawAnim();

	Light* light;

	float waterRotate;
	float waterCycle;

	bool winter;

	bool transitioning = false;
	float sceneTime;
	float transitionTime;
	float sceneStart = 0;
	bool hasTransitioned = false;
	Shader* transitionShader;

	Vector3 dragonLocations[4];
	int dragonLocation;
	Vector3 bearLocations[2];
	int bearLocation;
	const int totalMoveTime = 10;
	float curMoveTime;
	float Clamp(float value, float min, float max) { return (value < min) ? min : (value > max) ? max : value; }

	vector<Vector3> cameraPositions;
	vector<Vector2> cameraRotations;
	int numCameraPositions;
	int cameraPosition;
	float camMoveTime;
	float camPeriod;
	Vector3 InterpolateVector(const Vector3& vector1, const Vector3& vector2, float curMoveTime, float totalMoveTime);
	float InterpolateFloat(const float float1, const float float2, float curMoveTime, float totalMoveTime);
	float camRailBegun = false;
	bool camEndedManually = false;

	float snowFallSpeed = -10.0f;
	Vector3 dimensions;
};
