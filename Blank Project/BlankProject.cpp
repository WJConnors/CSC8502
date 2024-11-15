#include "../NCLGL/window.h"
#include "Renderer.h"

int main()	{
	Window w("Mountain Crater!", 1920, 1080, false);

	if(!w.HasInitialised()) {
		return -1;
	}
	
	Renderer renderer(w);
	if(!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	while(w.UpdateWindow()  && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1)) renderer.SwitchScene();
		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		renderer.RenderScene();
		renderer.SwapBuffers();
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_V)) {
			std::cout << renderer.GetCameraLocation();
			std::cout << "pitch: " << renderer.GetCameraPitch() << std::endl;
			std::cout << "yaw: " << renderer.GetCameraYaw() << std::endl;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_B)) {
			renderer.EndCameraRail();
		}
	}
	return 0;
}