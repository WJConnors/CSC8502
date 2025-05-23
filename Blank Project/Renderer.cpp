#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/CubeRobot.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
#include <algorithm>

float repeatFactor = 5.0f;
const int POST_PASSES = 5;
const int LIGHT_NUM = 256;
const int SNOW_NUM = 1024;
const int HEIGHTMULTIPLIER = 5;

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	heightMap = new HeightMap(TEXTUREDIR"MountainHM.png");
	camera = new Camera(-40, 270, Vector3());
	dimensions = heightMap->GetHeightmapSize();
	camera->SetPosition(dimensions * Vector3(0.5f, 0.3f, 0.5f));
	//light = new Light(dimensions * Vector3(0.5f, 1.5f, 0.5f), Vector4(0.373f,0.722f,0.741f,1), dimensions.x * 0.5f);
	light = new Light(dimensions * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), dimensions.x * 0.5f);
	quad = Mesh::GenerateQuad();
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	landscapeShader = new Shader("landscapeVertex.glsl", "landscapeFragment.glsl");
	if (!landscapeShader->LoadSuccess()) return;

	nodeShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	if (!nodeShader->LoadSuccess()) return;

	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!sceneShader->LoadSuccess()) return;

	processShader = new Shader("TexturedVertex.glsl", "processfrag.glsl");
	if (!processShader->LoadSuccess()) return;

	animShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
	if (!animShader->LoadSuccess()) return;

	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	if (!reflectShader->LoadSuccess()) return;

	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	if (!skyboxShader->LoadSuccess()) return;

	transitionShader = new Shader("TexturedVertex.glsl", "transitionFrag.glsl");
	if (!transitionShader->LoadSuccess()) return;


	mountainTex = SOIL_load_OGL_texture(TEXTUREDIR"snow2.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!mountainTex) return;
	mountainBump = SOIL_load_OGL_texture(TEXTUREDIR"snow2bump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!mountainBump) return;
	valleyTex = SOIL_load_OGL_texture(TEXTUREDIR"snow4.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!valleyTex) return;
	valleyBump = SOIL_load_OGL_texture(TEXTUREDIR"snow4bump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!valleyBump) return;

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR "water.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR "NLWest.png", TEXTUREDIR "NLEast.png",
		TEXTUREDIR "NLUp.png", TEXTUREDIR "NLDown.png",
		TEXTUREDIR "NLSouth.png", TEXTUREDIR "NLNorth.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	valleyTexSummer = SOIL_load_OGL_texture(TEXTUREDIR"rockValley.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!valleyTexSummer) return;
	valleyBumpSummer = SOIL_load_OGL_texture(TEXTUREDIR"rockValleyBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!valleyBumpSummer) return;

	SetTextureRepeating(mountainTex, true);
	SetTextureRepeating(mountainBump, true);
	SetTextureRepeating(valleyTex, true);
	SetTextureRepeating(valleyBump, true);
	SetTextureRepeating(valleyTexSummer, true);
	SetTextureRepeating(valleyBumpSummer, true);
	SetTextureRepeating(waterTex, true);

	cubeMapSummer = SOIL_load_OGL_cubemap(
		TEXTUREDIR "MountainWest.png", TEXTUREDIR "MountainEast.png",
		TEXTUREDIR "MountainUp.png", TEXTUREDIR "MountainDown.png",
		TEXTUREDIR "MountainSouth.png", TEXTUREDIR "MountainNorth.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	pointLights = new Light[LIGHT_NUM];
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		l.SetPosition(Vector3(rand() % (int)dimensions.x, rand() % (int)dimensions.y * HEIGHTMULTIPLIER, rand() % (int)dimensions.z));
		l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 1));
		l.SetRadius(250.0f + (rand() % 250));
	}

	gBufferShader = new Shader("landscapeVertex.glsl", "gFragment.glsl");
	pointLightShader = new Shader("pointlightvert.glsl", "pointlightfrag.glsl");
	combineShader = new Shader("combinevert.glsl", "combinefrag.glsl");
	if (!gBufferShader->LoadSuccess() || !pointLightShader->LoadSuccess() || !combineShader->LoadSuccess()) {
		return;
	}
	splitShader = new Shader("TexturedVertex.glsl", "splitFragment.glsl");
	if (!splitShader->LoadSuccess()) return;

	GenBuffers();

	dragonMesh = Mesh::LoadFromMeshFile("Dragon.msh");
	dragonAnim = new MeshAnimation("Dragon.anm");
	dragonMaterial = new MeshMaterial("Dragon.mat");

	for (int i = 0; i < dragonMesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = dragonMaterial->GetMaterialForLayer(i);
		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		dragonTextures.emplace_back(texID);
	}

	bearMesh = Mesh::LoadFromMeshFile("Bear.msh");
	bearAnim = new MeshAnimation("Bear.anm");
	bearMaterial = new MeshMaterial("Bear.mat");

	for (int i = 0; i < bearMesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = bearMaterial->GetMaterialForLayer(i);
		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		bearTextures.emplace_back(texID);
	}
	curDragFrame = 0;
	dragFrameTime = 0.0f;
	curBearFrame = 0;
	bearFrameTime = 0;
	sceneTime = 0;
	transitionTime = 0.0f;
	splitScreenTime = 0.0f;

	root = new SceneNode();
	root->SetTransform(Matrix4::Translation(Vector3(0,0,0)));

	for (int i = 0; i < SNOW_NUM; i++) {
		SceneNode* snow = new SceneNode(Mesh::LoadFromMeshFile("Sphere.msh"));
		snow->SetModelScale(Vector3(2.0f, 2.0f, 2.0f));
		snow->SetBoundingRadius(2.0f);
		snow->SetTransform(Matrix4::Translation(
			Vector3(rand() % (int)dimensions.x, rand() % (int)dimensions.y * HEIGHTMULTIPLIER, rand() % (int)dimensions.z)));
		snow->SetTexture(mountainTex);
		root->AddChild(snow);
	}

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	dragonLocations[0] = Vector3(2570, 151, 2600);
	dragonLocations[1] = Vector3(1640, 151, 2600);
	dragonLocations[2] = Vector3(1640, 151, 1500);
	dragonLocations[3] = Vector3(2570, 151, 1500);
	dragonLocation = 0;

	bearLocations[0] = Vector3(2313, 20, 1144);
	bearLocations[1] = Vector3(2655, 20, 1357);
	bearLocation = 0;

	curMoveTime = 0;

	cameraPositions.emplace_back(Vector3(-394.879, 414.266, 1991.06));
	cameraRotations.emplace_back(Vector2(8.57997, 270.83));
	cameraPositions.emplace_back(Vector3(412.497, 414.266, 1990.4));
	cameraRotations.emplace_back(Vector2(-11.51, 269.71));
	cameraPositions.emplace_back(Vector3(638.709, 291.988, 940.589));
	cameraRotations.emplace_back(Vector2(-9.97003, 248.2871));
	cameraPositions.emplace_back(Vector3(3101.56, 189.129, 800.311));
	cameraRotations.emplace_back(Vector2(-12.84, 146.017));
	cameraPositions.emplace_back(Vector3(3021.49, 80.5835, 875.656));
	cameraRotations.emplace_back(Vector2(-0.530248, 165.069));
	cameraPositions.emplace_back(Vector3(3022.17, 80.5835, 1198.05));
	cameraRotations.emplace_back(Vector2(-7.86976, 179.419));

	cameraPositions.emplace_back(Vector3(3093.66, 80.5835, 1085.27));
	cameraRotations.emplace_back(Vector2(-1.00783, 124.114));
	cameraPositions.emplace_back(Vector3(2727.43, 38.8582, 1380.45));
	cameraRotations.emplace_back(Vector2(-8.21784, 68.1846));
	cameraPositions.emplace_back(Vector3(2407.07, 38.8582, 1197.43));
	cameraRotations.emplace_back(Vector2(-7.37784, 60.3447));
	cameraPositions.emplace_back(Vector3(2360, 38.8582, 1226.55));
	cameraRotations.emplace_back(Vector2(1.93216, 170.385));
	cameraPositions.emplace_back(Vector3(2155.89, 433.61, 2321.01));
	cameraRotations.emplace_back(Vector2(-19.0678, 58.5947));
	cameraPositions.emplace_back(Vector3(951.816, 1057.62, 3259.86));
	cameraRotations.emplace_back(Vector2(-34.7477, 322.622));

	numCameraPositions = cameraPositions.size();
	cameraPosition = 0;
	camera->SetPosition(cameraPositions[cameraPosition]);
	camera->SetPitch(cameraRotations[cameraPosition].x);
	camera->SetYaw(cameraRotations[cameraPosition].y);
	camMoveTime = 0;
	camPeriod = 5;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	winter = true;
	init = true;
}
Renderer::~Renderer(void)	{
	delete root;
	delete heightMap;
	delete camera;
	delete landscapeShader;
	glDeleteTextures(1, &mountainTex);
	glDeleteTextures(1, &valleyTex);
	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &blurFBO);

	delete dragonMesh;
	delete dragonAnim;
	delete dragonMaterial;

	delete light;

	delete reflectShader;
	delete skyboxShader;
	delete nodeShader;
	delete sceneShader;
	delete processShader;
	delete animShader;

	delete gBufferShader;
	delete pointLightShader;
	delete combineShader;
	delete sphere;
	delete quad;
	delete[] pointLights;

	glDeleteTextures(1, &gBufferColourTex);
	glDeleteTextures(1, &gBufferNormalTex);
	glDeleteTextures(1, &gBufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

	glDeleteFramebuffers(1, &gBufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);
}

void Renderer::UpdateScene(float dt) {

	if (sceneStart == 0) sceneStart = dt;
	if (camEnded || camEndedManually) {
		camera->UpdateCamera(dt);
	}
	else {
		if (!camRailBegun || splitScreen) {
			camRailBegun = true;
		}
		else {
			Vector3 curStart = cameraPositions[cameraPosition];
			Vector3 curEnd = cameraPositions[cameraPosition + 1];
			camera->SetPosition(InterpolateVector(curStart, curEnd, camMoveTime, camPeriod));
			Vector2 rotStart = cameraRotations[cameraPosition];
			Vector2 rotEnd = cameraRotations[cameraPosition + 1];
			camera->SetPitch(InterpolateFloat(rotStart.x, rotEnd.x, camMoveTime, camPeriod));
			camera->SetYaw(InterpolateFloat(rotStart.y, rotEnd.y, camMoveTime, camPeriod));
			camMoveTime += dt;
			if (camMoveTime >= camPeriod) {
				camMoveTime -= camPeriod;
				cameraPosition++;
			}
			if (cameraPosition == numCameraPositions - 1) {
				splitScreen = true;
			}
		}
	}

	if (splitScreen) {
		splitScreenTime += dt;
		if (splitScreenTime >= splitScreenMaxTime) {
			splitScreen = false;
			camEnded = true;
		}
	}

	viewMatrix = camera->BuildViewMatrix();

	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
	sceneTime += dt;
	if (sceneStart == 0) sceneStart = sceneTime;

	if (sceneTime > sceneStart + 25.0f && sceneTime < sceneStart + 30.0f) {
		transitioning = true;
		transitionTime += dt;
		if (transitionTime > 2.5f && !hasTransitioned) {
			winter = !winter;
			hasTransitioned = true;
		}
	}
	else {
		transitioning = false;
	}

	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	dragFrameTime -= dt;
	while (dragFrameTime < 0.0f) {
		curDragFrame = (curDragFrame + 1) % dragonAnim->GetFrameCount();
		dragFrameTime += 1.0f / dragonAnim->GetFrameRate();
	}

	bearFrameTime -= dt;
	while (bearFrameTime < 0.0f) {
		curBearFrame = (curBearFrame + 1) % bearAnim->GetFrameCount();
		bearFrameTime += 1.0f / bearAnim->GetFrameRate();
	}

	curMoveTime += dt;
	if (curMoveTime >= totalMoveTime) {
		curMoveTime -= totalMoveTime;
		
		dragonLocation = (dragonLocation + 1) % 4;
		bearLocation = (bearLocation + 1) % 2;
	}

	for (vector < SceneNode* >::const_iterator
		i = root->GetChildIteratorStart();
		i != root->GetChildIteratorEnd(); ++i) {
		(*i)->SetTransform((*i)->GetTransform() * Matrix4::Translation(Vector3(0, snowFallSpeed * dt, 0)));
		Vector3 curTransform = (*i)->GetTransform().GetPositionVector();
		if (curTransform.y < 0) {
			(*i)->SetTransform((*i)->GetTransform() * Matrix4::Translation(Vector3(curTransform.x, dimensions.y * HEIGHTMULTIPLIER, curTransform.y)));
		}
	}
	root->Update(dt);
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() *
			Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(gBufferShader->GetProgram(),"modelMatrix"), 1, false, model.values);

		n->Draw(*this);
	}
	for (vector < SceneNode* >::const_iterator
		i = n->GetChildIteratorStart();
		i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}

void Renderer::RenderScene() {
	if (!splitScreen) {
		DrawScene();
		PresentScene();
	}
	else {
		DrawScene();
		PresentSplitScene(false);
		winter = !winter;
		DrawScene();
		PresentSplitScene(true);
		winter = !winter;
	}
}

void Renderer::DrawScene() {

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	viewMatrix = camera->BuildViewMatrix();
	UpdateShaderMatrices();

	DrawSkybox();
	DrawHeightMap();
	
	if (winter) { //Winter scene
		DrawNode(root); //Draw main geometry

		DrawPointLights(); //Lights drawn seperately for deferred rendering
		CombineBuffers();
		DrawBlur();
	}
	else { //Summer Scene
		DrawWater();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	viewMatrix = camera->BuildViewMatrix();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	if (winter) DrawAnim();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawBlur() {
	glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		quad->Draw();

		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentSplitScene(bool output) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	BindShader(splitShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(splitShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(splitShader->GetProgram(), "output"), output);
	glUniform1f(glGetUniformLocation(splitShader->GetProgram(), "width"), (float)width / 2);

	quad->Draw();
}

void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Shader* curShader = transitioning ? transitionShader : sceneShader;
	BindShader(curShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	if (transitioning) {
		glUniform1f(glGetUniformLocation(curShader->GetProgram(), "transitionTime"), transitionTime);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(curShader->GetProgram(), "diffuseTex"), 0);

	quad->Draw();
}

void Renderer::GenBuffers() {
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO);
	

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) {
		return;
	}

	glGenFramebuffers(1, &blurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);

	glGenTextures(1, &blurDepthTex);
	glBindTexture(GL_TEXTURE_2D, blurDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, blurDepthTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Blur framebuffer is not complete!" << std::endl;
	}

	glGenFramebuffers(1, &gBufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	GenerateScreenTexture(gBufferDepthTex, true);
	GenerateScreenTexture(gBufferColourTex);
	GenerateScreenTexture(gBufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gBufferDepthTex, 0);
	glDrawBuffers(2, buffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return;

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::DrawHeightMap() {
	Shader* curShader;
	if (winter) {
		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		BindShader(gBufferShader);
		curShader = gBufferShader;
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
		BindShader(landscapeShader);
		curShader = landscapeShader;
		glUniform3fv(glGetUniformLocation(landscapeShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
		SetShaderLight(*light);
	}

	textureMatrix = Matrix4::Scale(Vector3(repeatFactor, repeatFactor, 1.0f));
	modelMatrix.ToIdentity();
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(curShader->GetProgram(), "mountainTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, winter ? mountainTex : valleyTexSummer);

	glUniform1i(glGetUniformLocation(curShader->GetProgram(), "valleyTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, winter ? valleyTex : valleyTexSummer);

	glUniform1i(glGetUniformLocation(curShader->GetProgram(), "mountainBump"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, winter ? mountainBump : valleyBumpSummer);

	glUniform1i(glGetUniformLocation(curShader->GetProgram(), "valleyBump"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, winter ? valleyBump : valleyBumpSummer);

	glUniform1f(glGetUniformLocation(curShader->GetProgram(), "heightThreshold"), 60.0f);
	glUniform1f(glGetUniformLocation(curShader->GetProgram(), "transitionWidth"), 10.0f);

	heightMap->Draw();
}

void Renderer::DrawAnim() {
	BindShader(animShader);
	glClear(GL_DEPTH_BUFFER_BIT);
	Vector3 pos1;
	Vector3 pos2;
	if (winter) {
		pos1 = dragonLocations[dragonLocation];
		pos2 = dragonLocation == 3 ? dragonLocations[0] : dragonLocations[dragonLocation + 1];
	}
	else {
		pos1 = bearLocations[bearLocation];
		pos2 = bearLocation == 1 ? bearLocations[0] : bearLocations[bearLocation + 1];
	}
	Vector3 interpolatedVector = InterpolateVector(pos1, pos2, curMoveTime, totalMoveTime);
	modelMatrix = Matrix4::Translation(interpolatedVector);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(animShader->GetProgram(), "diffuseTex"), 0);
	vector<Matrix4> frameMatrices;
	Mesh* curMesh;
	MeshAnimation* curAnim;
	MeshMaterial* curMat;
	int curFrame;
	if (winter) {
		curMesh = dragonMesh;
		curAnim = dragonAnim;
		curMat = dragonMaterial;
		curFrame = curDragFrame;
	}
	else {
		curMesh = bearMesh;
		curAnim = bearAnim;
		curMat = bearMaterial;
		curFrame = curBearFrame;
	}

	const Matrix4* invBindPose = curMesh->GetInverseBindPose();
	const Matrix4* frameData = curAnim->GetJointData(curFrame);

	for (unsigned int i = 0; i < curMesh->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(animShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < curMesh->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, winter ? dragonTextures[i] : bearTextures[i]);
		curMesh->DrawSubMesh(i);
	}
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	if (winter) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
		glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 0);
	}
	else {
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapSummer);
		glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 0);
	}

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawWater() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapSummer);

	Vector3 hSize = heightMap->GetHeightmapSize();
	modelMatrix =
		Matrix4::Translation(hSize * Vector3(0.5f, 0.12f, 0.5f)) *
		Matrix4::Scale(hSize * 0.5f) *
		Matrix4::Rotation(-90, Vector3(1, 0, 0));

	textureMatrix =
		Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
		Matrix4::Scale(Vector3(10, 10, 10)) *
		Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
}

void Renderer::DrawPointLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointLightShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointLightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBufferDepthTex);

	glUniform1i(glGetUniformLocation(pointLightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBufferNormalTex);

	glUniform3fv(glGetUniformLocation(pointLightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform2f(glGetUniformLocation(pointLightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointLightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	UpdateShaderMatrices();
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glClearColor(0.2f, 0.2f, 0.2f, 1);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CombineBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	BindShader(combineShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBufferColourTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "depthTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gBufferDepthTex);

	quad->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Vector3 Renderer::InterpolateVector(const Vector3& vector1, const Vector3& vector2, float curMoveTime, float totalMoveTime) {
	float t = curMoveTime / totalMoveTime;     // Calculate interpolation factor
	t = Clamp(t, 0.0f, 1.0f);            // Clamp t to [0.0, 1.0]
	return vector1 * (1.0f - t) + vector2 * t; // Perform interpolation
}

float Renderer::InterpolateFloat(const float float1, const float float2, float curMoveTime, float totalMoveTime) {
	float t = curMoveTime / totalMoveTime;
	t = Clamp(t, 0.0f, 1.0f);
	return float1 * (1.0f - t) + float2 * t;
}