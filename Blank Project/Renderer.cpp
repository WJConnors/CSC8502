#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"

float repeatFactor = 5;

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	heightMap = new HeightMap(TEXTUREDIR"manualHM.png");
	camera = new Camera(-40, 270, Vector3());
	Vector3 dimensions = heightMap->GetHeightmapSize();
	camera->SetPosition(dimensions * Vector3(0.5, 0.1, 0.5));

	shader = new Shader("landscapeVertex.glsl", "landscapeFragment.glsl");
	if (!shader->LoadSuccess()) return;

	mountainTex = SOIL_load_OGL_texture(TEXTUREDIR"snow2.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!mountainTex) return;
	valleyTex = SOIL_load_OGL_texture(TEXTUREDIR"snow4.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!mountainTex) return;

	SetTextureRepeating(mountainTex, true);
	SetTextureRepeating(valleyTex, true);
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	init = true;
}
Renderer::~Renderer(void)	{
	delete heightMap;
	delete camera;
	delete shader;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::RenderScene()	{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BindShader(shader);
	UpdateShaderMatrices();

	textureMatrix = Matrix4::Scale(Vector3(repeatFactor, repeatFactor, 1.0f));

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "mountainTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mountainTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "valleyTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, valleyTex);

	glUniform1f(glGetUniformLocation(shader->GetProgram(), "heightThreshold"), 10.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "transitionWidth"), 5.0f);

	heightMap->Draw();
}

