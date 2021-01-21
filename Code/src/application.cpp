#include "application.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "volume.h"
#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "animation.h"
#include "extra/hdre.h"
#include "extra/imgui/imgui.h"
#include "extra/imgui/imgui_impl_sdl.h"
#include "extra/imgui/imgui_impl_opengl3.h"

#include "light.h"

#include <cmath>

Application* Application::instance = NULL;
Camera* Application::camera = nullptr;

float cam_speed = 10;
bool render_wireframe = false;

//Texturas, shaders, materials, variables, meshes...
Volume* vrVolume = NULL;
VRMaterial* vrMaterial = NULL;
Shader* vrShader = NULL;
Mesh* vrMesh = NULL;
Texture* vrTexture = NULL;
float step_Length = 0.0012f;

//Jittering
Texture* jittering = NULL;

//LUT
Texture* LUT = NULL;
float sl1 = 0.26; //slider 1
float sl2 = 0.30; //slider 2

//Gradient
float th = 0.35; //threshold 
float h = 0.01; //h value
Light* light = NULL;

//Clipping
vec4 clipping = vec4(0.0, 1.0, 0.0, -0.752);

//ImGui
bool jit_bool = true;
bool part1 = true;
bool part2 = false;
bool part2_text = true;
bool part2_sliders = false;
bool gradient_option = false;


Application::Application(int window_width, int window_height, SDL_Window* window)
{
	this->window_width = window_width;
	this->window_height = window_height;
	this->window = window;
	instance = this;
	must_exit = false;
	render_debug = true;

	fps = 0;
	frame = 0;
	time = 0.0f;
	elapsed_time = 0.0f;
	mouse_locked = false;

	// OpenGL flags
	glEnable( GL_CULL_FACE ); //render both sides of every triangle
	glEnable( GL_DEPTH_TEST ); //check the occlusions using the Z buffer

	// Create camera
	camera = new Camera();
	camera->lookAt(Vector3(-5.f, 1.5f, 10.f), Vector3(0.f, 0.0f, 0.f), Vector3(0.f, 1.f, 0.f));
	camera->setPerspective(45.f,window_width/(float)window_height,0.1f,10000.f); //set the projection, we want to be perspective
	camera->eye = vec3(-1577.61, 861.74, 1126.88);
	

	// Create node and add it to the scene
	SceneNode * node = new SceneNode("Scene node VRMaterial");
	node_list.push_back(node);

	//Create Mesh
	Mesh* vrMesh = new Mesh();
	vrMesh->createCube();
	node->mesh = vrMesh;

	//Create shader
	vrShader = new Shader();
	vrShader = Shader::Get("data/shaders/basic.vs", "data/shaders/VR.fs");

	//Create Volume
	vrVolume = new Volume();
	vrVolume->loadPVM("data/volumes/CT-Abdomen.pvm");

	node->model.setScale(vrVolume->width*vrVolume->widthSpacing, vrVolume->height*vrVolume->heightSpacing, vrVolume->depth*vrVolume->depthSpacing);

	//Create Texture
	vrTexture = new Texture();
	vrTexture->create3DFromVolume(vrVolume);

	//Jittering
	jittering = new Texture();
	jittering = Texture::Get("data/maps/noise.png");

	//LUT
	LUT = new Texture();
	LUT = Texture::Get("data/maps/LUT_RGBA.png");

	//Light
	light = new Light();

	//hide the cursor
	SDL_ShowCursor(!mouse_locked); //hide or show the mouse
}

//what to do when the image has to be draw
void Application::render(void)
{
	//set the clear color (the background color)
	glClearColor(0, 0, 0, 1);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera as default
	camera->enable();

	for (int i = 0; i < node_list.size(); i++) {
		node_list[i]->render(camera);

		if(render_wireframe)
			node_list[i]->renderWireframe(camera);
	}

	//Draw the floor grid
	//if(render_debug)
		//drawGrid();
}

void Application::update(double seconds_elapsed)
{
	mouse_locked = false;
	float speed = seconds_elapsed * cam_speed; //the speed is defined by the seconds_elapsed so it goes constant
	float orbit_speed = seconds_elapsed * 0.5f;
	
	//camera speed modifier
	if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) speed *= 10; //move faster with left shift

	float pan_speed = speed * 0.5f;

	//async input to move the camera around
	if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::isKeyPressed(SDL_SCANCODE_UP))		camera->move(Vector3( 0.0f, 0.0f,  1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::isKeyPressed(SDL_SCANCODE_DOWN))	camera->move(Vector3( 0.0f, 0.0f, -1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::isKeyPressed(SDL_SCANCODE_LEFT))	camera->move(Vector3( 1.0f, 0.0f,  0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::isKeyPressed(SDL_SCANCODE_RIGHT)) camera->move(Vector3(-1.0f, 0.0f,  0.0f) * speed);


	// Set material
	vrMaterial = new VRMaterial(jit_bool, part1, part2, part2_text, part2_sliders, gradient_option, vrShader, vrTexture, step_Length, sl1, sl2, jittering, LUT, light, th, h, clipping);
	node_list[0]->material = vrMaterial;


	if (!HoveringImGui()) 
	{
		//move in first person view
		if (mouse_locked || Input::mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT))
		{
			mouse_locked = true;
			camera->rotate(-Input::mouse_delta.x * orbit_speed * 0.5, Vector3(0, 1, 0));
			Vector3 right = camera->getLocalVector(Vector3(1, 0, 0));
			camera->rotate(-Input::mouse_delta.y * orbit_speed * 0.5, right);
		}

		//orbit around center
		else if (Input::mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) //is left button pressed?
		{
			mouse_locked = true;
			camera->orbit(-Input::mouse_delta.x * orbit_speed, Input::mouse_delta.y * orbit_speed);
		}

		//camera panning
		else if(Input::mouse_state& SDL_BUTTON(SDL_BUTTON_MIDDLE)) 
		{
				mouse_locked = true;
				camera->move(Vector3(-Input::mouse_delta.x * pan_speed, 0.f, 0.f));
				camera->move(Vector3(0.f, Input::mouse_delta.y * pan_speed, 0.f));
		}
	}

	//move up or down the camera using Q and E keys
	if (Input::isKeyPressed(SDL_SCANCODE_Q) || Input::isKeyPressed(SDL_SCANCODE_SPACE)) camera->moveGlobal(Vector3(0.0f, -1.0f, 0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_E) || Input::isKeyPressed(SDL_SCANCODE_LCTRL)) camera->moveGlobal(Vector3(0.0f,  1.0f, 0.0f) * speed);
	
	//to navigate with the mouse fixed in the middle
	if (mouse_locked)
		Input::centerMouse();

	SDL_ShowCursor(!mouse_locked);
	ImGui::SetMouseCursor(mouse_locked ? ImGuiMouseCursor_None : ImGuiMouseCursor_Arrow);
}

void Application::renderInMenu()
{
		ImGui::NewLine();

		//Step length
		ImGui::Text("Step Length");
		ImGui::InputFloat("S.L.", &step_Length, 0.0001f, 1.0f, "%.5f");
		
		ImGui::NewLine();
		ImGui::Checkbox("Jittering", &jit_bool);

		ImGui::NewLine();

		//-------------------ACTIVATE CHECKBOX OF PART 1-------------------------------
		ImGui::Checkbox("Part 1", &part1);

		if (part1 == true) {

			part2 = false;

		}

		ImGui::NewLine();

		//-------------------ACTIVATE CHECKBOX OF PART 2-------------------------------
		ImGui::Checkbox("Part 2", &part2);
		if (part2 == true) {
			part1 = false;
			ImGui::NewLine();
			if (ImGui::TreeNode("COLOR")) {
				ImGui::NewLine();
				ImGui::Checkbox("Color Texture LUT", &part2_text);
				if (part2_text == true) {
					part2_sliders = false;
				}
				ImGui::NewLine();
				ImGui::Checkbox("Color Sliders", &part2_sliders);
				if (part2_sliders == true) {
					ImGui::NewLine();
					part2_text = false;
					ImGui::Text("Red");
					ImGui::SliderFloat("R", (float*)&sl1, 0.0f, 1.0f, "%.5f");
					ImGui::Text("Green");
					ImGui::SliderFloat("G", (float*)&sl2, 0.0f, 1.0f, "%.5f");
				}
				ImGui::TreePop();
			}

			ImGui::NewLine();

			if (ImGui::TreeNode("GRADIENT OPTION")) {
				ImGui::NewLine();
				ImGui::Checkbox("Activate", &gradient_option);
				if (gradient_option == true) {
					ImGui::NewLine();
					ImGui::Text("Threshold");
					ImGui::InputFloat("th", &th, 0.001f, 1.0f, "%.4f");
					ImGui::Text("h value");
					ImGui::InputFloat("h", &h, 0.0001f, 1.0f, "%.5f");
					ImGui::NewLine();
					if (ImGui::TreeNode("LIGHT")) {
						ImGui::NewLine();
						ImGui::Text("Position");
						ImGui::DragFloat3("P", (float*)&light->position, 0.1f);//position
						ImGui::SliderFloat("X", (float*)&light->position.x, -250.0f, 250.0f, "%.4f");
						ImGui::SliderFloat("Y", (float*)&light->position.y, -250.0f, 250.0f, "%.4f");
						ImGui::SliderFloat("Z", (float*)&light->position.z, -250.0f, 250.0f, "%.4f");
						ImGui::Text("Color");
						ImGui::ColorEdit3("RGB", (float*)&light->color); // Edit 3 floats representing a color
						ImGui::Text("Intensity");
						ImGui::DragFloat("I", (float*)&light->intensity, 0.05f, 0.0f, 10.0f);//intensity
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}

			ImGui::NewLine();
			//Clipping plane
			ImGui::Text("Clipping plane");
			ImGui::DragFloat4("C.P.", (float*)&clipping, 0.001f);

			ImGui::NewLine();

		}
		ImGui::NewLine();
}

//Keyboard event handler (sync input)
void Application::onKeyDown( SDL_KeyboardEvent event )
{
	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: must_exit = true; break; //ESC key, kill the app
		case SDLK_F1: render_debug = !render_debug; break;
		case SDLK_F2: render_wireframe = !render_wireframe; break;
		case SDLK_F5: Shader::ReloadAll(); break; 
	}
}

void Application::onKeyUp(SDL_KeyboardEvent event)
{
}

void Application::onGamepadButtonDown(SDL_JoyButtonEvent event)
{

}

void Application::onGamepadButtonUp(SDL_JoyButtonEvent event)
{

}

void Application::onMouseButtonDown( SDL_MouseButtonEvent event )
{

}

void Application::onMouseButtonUp(SDL_MouseButtonEvent event)
{
}

void Application::onMouseWheel(SDL_MouseWheelEvent event)
{
	bool mouse_blocked = false;

	ImGuiIO& io = ImGui::GetIO();
	if (!mouse_locked)
		switch (event.type)
		{
		case SDL_MOUSEWHEEL:
		{
			if (event.x > 0) io.MouseWheelH += 1;
			if (event.x < 0) io.MouseWheelH -= 1;
			if (event.y > 0) io.MouseWheel += 1;
			if (event.y < 0) io.MouseWheel -= 1;
		}
		}
	mouse_blocked = ImGui::IsAnyWindowHovered();

	if (!mouse_blocked && event.y)
	{
		if (mouse_locked)
			cam_speed *= 1 + (event.y * 0.1);
		else
			camera->changeDistance(event.y * 0.5);
	}
}

void Application::onResize(int width, int height)
{
  std::cout << "window resized: " << width << "," << height << std::endl;
	glViewport( 0,0, width, height );
	camera->aspect =  width / (float)height;
	window_width = width;
	window_height = height;
}

