#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/CubeRobot.h"
#include <algorithm>

float repeatFactor = 5;

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	heightMap = new HeightMap(TEXTUREDIR"manualHM.png");
	camera = new Camera(-40, 270, Vector3());
	Vector3 dimensions = heightMap->GetHeightmapSize();
	camera->SetPosition(dimensions * Vector3(0.5, 0.1, 0.5));
	quad = Mesh::GenerateQuad();

	landscapeShader = new Shader("landscapeVertex.glsl", "landscapeFragment.glsl");
	if (!landscapeShader->LoadSuccess()) return;

	nodeShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	if (!nodeShader->LoadSuccess()) return;

	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!sceneShader->LoadSuccess()) return;

	mountainTex = SOIL_load_OGL_texture(TEXTUREDIR"snow2.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!mountainTex) return;
	valleyTex = SOIL_load_OGL_texture(TEXTUREDIR"snow4.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!valleyTex) return;

	SetTextureRepeating(mountainTex, true);
	SetTextureRepeating(valleyTex, true);

	GenBuffers();

	root = new SceneNode();

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	init = true;
}
Renderer::~Renderer(void)	{
	delete root;
	delete heightMap;
	delete camera;
	delete landscapeShader;
	glDeleteTextures(1, &mountainTex);
	glDeleteTextures(1, &valleyTex);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	root->Update(dt);
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}
	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) {
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() *
			Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(
			glGetUniformLocation(nodeShader->GetProgram(),
				"modelMatrix"), 1, false, model.values);
		glUniform4fv(glGetUniformLocation(nodeShader->GetProgram(),
			"nodeColour"), 1, (float*)&n->GetColour());
		GLuint texture = n->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(nodeShader->GetProgram(),
			"useTexture"), texture);
		n->Draw(*this);
	}
}

void Renderer::RenderScene() {
	DrawScene();
	//PresentScene();
}

void Renderer::DrawScene() {
	//glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BindShader(landscapeShader);
	UpdateShaderMatrices();

	textureMatrix = Matrix4::Scale(Vector3(repeatFactor, repeatFactor, 1.0f));

	glUniform1i(glGetUniformLocation(landscapeShader->GetProgram(), "mountainTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mountainTex);

	glUniform1i(glGetUniformLocation(landscapeShader->GetProgram(), "valleyTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, valleyTex);

	glUniform1f(glGetUniformLocation(landscapeShader->GetProgram(), "heightThreshold"), 10.0f);
	glUniform1f(glGetUniformLocation(landscapeShader->GetProgram(), "transitionWidth"), 5.0f);

	heightMap->Draw();

	BuildNodeLists(root);
	SortNodeLists();
	BindShader(nodeShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(nodeShader->GetProgram(), "diffuseTex"), 0);
	DrawNodes();
	ClearNodeLists();

	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::PresentScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BindShader(sceneShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);

	quad->Draw();
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::GenBuffers() {
	glGenFramebuffers(1, &bufferFBO);
	glGenTextures(1, &bufferColourTex);

	glBindTexture(GL_TEXTURE_2D, bufferColourTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}