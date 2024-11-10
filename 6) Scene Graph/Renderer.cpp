#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	camera = new Camera();
	landscapeShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	if (!landscapeShader->LoadSuccess()) {
		return;		
	}

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f,
	(float)width / (float)height, 45.0f);
	camera->SetPosition(Vector3(0, 30, 175));
	root = new SceneNode();
	root->AddChild(new CubeRobot(cube));
	glEnable(GL_DEPTH_TEST);
	init = true;
}

Renderer ::~Renderer(void) {
	delete root;
	delete landscapeShader;
	delete camera;
	delete cube;	
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	root->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(landscapeShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(landscapeShader->GetProgram(),
	 "diffuseTex"), 1);
	DrawNode(root);	
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() *
			Matrix4::Scale(n-> GetModelScale());
		glUniformMatrix4fv(
			glGetUniformLocation(landscapeShader->GetProgram(),
			"modelMatrix"), 1, false, model.values);
		//Vector4 temp = n->GetColour();
		auto temp = (float*)&n->GetColour();
		float r = temp[0];
		float g = temp[1];
		float b = temp[2];
		float a = temp[3];
		glUniform4fv(glGetUniformLocation(landscapeShader->GetProgram(),
			"nodeColour"), 1, (float*)&n->GetColour());
		glUniform1i(glGetUniformLocation(landscapeShader->GetProgram(),
			"useTexture"), 0);
		n->Draw(*this);
	}
	for (vector < SceneNode* >::const_iterator
		i = n-> GetChildIteratorStart();
		i != n-> GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}