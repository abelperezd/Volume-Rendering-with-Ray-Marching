/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com

	MAIN:
	 + This file creates the window and the game instance. 
	 + It also contains the mainloop
	 + This is the lowest level, here we access the system to create the opengl Context
	 + It takes all the events from SDL and redirect them to the game
*/

#include "includes.h"

#include "framework.h"
#include "mesh.h"
#include "camera.h"
#include "utils.h"
#include "input.h"
#include "application.h"

#include <iostream> //to output

long last_time = 0; //this is used to calcule the elapsed time between frames

Application* game = NULL;
SDL_GLContext glcontext;

// *********************************
//create a window using SDL
SDL_Window* createWindow(const char* caption, int width, int height, bool fullscreen = false)
{
    int multisample = 8;
    bool retina = true; //change this to use a retina display

	//set attributes
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16); //or 24
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	//antialiasing (disable this lines if it goes too slow)
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisample ); //increase to have smoother polygons

	// Initialize the joystick subsystem
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	//create the window
	SDL_Window * sdl_window = SDL_CreateWindow(caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|
                                          (retina ? SDL_WINDOW_ALLOW_HIGHDPI:0) |
                                          (fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0) );
	if(!sdl_window)
	{
		fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
		exit(-1);
	}
  
	// Create an OpenGL context associated with the window.
	glcontext = SDL_GL_CreateContext(sdl_window);

	//in case of exit, call SDL_Quit()
	atexit(SDL_Quit);

	//get events from the queue of unprocessed events
	SDL_PumpEvents(); //without this line asserts could fail on windows

	//launch glew to extract the opengl extensions functions from the DLL
	#ifdef USE_GLEW
		glewInit();
	#endif

	int window_width, window_height;
	SDL_GetWindowSize(sdl_window, &window_width, &window_height);
	std::cout << " * Window size: " << window_width << " x " << window_height << std::endl;
	std::cout << " * Path: " << getPath() << std::endl;
	std::cout << std::endl;

	return sdl_window;
}

void renderGUI(SDL_Window* window, Application * game)
{
	
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.MousePos.x = Input::mouse_position.x;
	io.MousePos.y = Input::mouse_position.y;

	assert(window);

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();

	{
		ImGui::Begin("Debugger");                          // Create a window called "Hello, world!" and append into it.

		//System stats
		ImGui::Text(getGPUStats().c_str());					   // Display some text (you can use a format strings too)
		
		if (ImGui::TreeNode("VR Parameters ")) {
			game->renderInMenu();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Camera")) {
			game->camera->renderInMenu();
			ImGui::TreePop();
		}

		//Scene graph
		if (ImGui::TreeNode("Entities"))
		{
			unsigned int count = 0;
			std::stringstream ss;
			for (auto& node : game->node_list)
			{
				ss << count;
				if ( ImGui::TreeNode(node->name.c_str()) )
				{
					node->renderInMenu();
					ImGui::TreePop();
				}
				++count;
				ss.str("");
			}
			ImGui::TreePop();
		}
		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

//The application main loop
void mainLoop(SDL_Window* window)
{
	SDL_Event sdlEvent;

	long start_time = SDL_GetTicks();
	long now = start_time;
	long frames_this_second = 0;

	while (!game->must_exit)
	{
		//update events
		while(SDL_PollEvent(&sdlEvent))
		{
			Input::update();

			switch (sdlEvent.type)
			{
			case SDL_QUIT: return; break; //EVENT for when the user clicks the [x] in the corner
			case SDL_MOUSEBUTTONDOWN: //EXAMPLE OF sync mouse input
				Input::mouse_state |= SDL_BUTTON(sdlEvent.button.button);
				game->onMouseButtonDown(sdlEvent.button);
				break;
			case SDL_MOUSEBUTTONUP:
				Input::mouse_state &= ~SDL_BUTTON(sdlEvent.button.button);
				game->onMouseButtonUp(sdlEvent.button);
				break;
			case SDL_MOUSEWHEEL:
				Input::mouse_wheel += sdlEvent.wheel.y;
				Input::mouse_wheel_delta = sdlEvent.wheel.y;
				game->onMouseWheel(sdlEvent.wheel);
				break;
			case SDL_KEYDOWN:
				game->onKeyDown(sdlEvent.key);
				break;
			case SDL_KEYUP:
				game->onKeyUp(sdlEvent.key);
				break;
			case SDL_JOYBUTTONDOWN:
				game->onGamepadButtonDown(sdlEvent.jbutton);
				break;
			case SDL_JOYBUTTONUP:
				game->onGamepadButtonUp(sdlEvent.jbutton);
				break;
			case SDL_TEXTINPUT:
				// you can read the ASCII character from sdlEvent.text.text 
				break;
			case SDL_WINDOWEVENT:
				switch (sdlEvent.window.event) {
				case SDL_WINDOWEVENT_RESIZED: //resize opengl context
					game->onResize(sdlEvent.window.data1, sdlEvent.window.data2);
					break;
				}
			}
		}


		// swap between front buffer and back buffer
		SDL_GL_SwapWindow(window);

		//compute delta time
		long last_time = now;
		now = SDL_GetTicks();
		double elapsed_time = (now - last_time) * 0.001; //0.001 converts from milliseconds to seconds
		double last_time_seconds = game->time;
        game->time = float(now * 0.001);
		game->elapsed_time = elapsed_time;
		game->frame++;
		frames_this_second++;
		if (int(last_time_seconds *2) != int(game->time*2)) //next half second
		{
			game->fps = frames_this_second*2;
			frames_this_second = 0;
		}

		//update game logic
		game->update(elapsed_time);

		//render frame
		game->render();

		renderGUI(window, game);

		//check errors in opengl only when working in debug
		#ifdef _DEBUG
				checkGLErrors();
		#endif
	}

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(game->window);
	SDL_Quit();

	return;
}

int main(int argc, char **argv)
{
	std::cout << "Initiating engine..." << std::endl;

	//prepare SDL
	SDL_Init(SDL_INIT_EVERYTHING);

	bool fullscreen = false; //change this to go fullscreen
	Vector2 size(1024,768);

	if(fullscreen)
		size = getDesktopSize(0);

	//create the game window (WINDOW_WIDTH and WINDOW_HEIGHT are two macros defined in includes.h)
	SDL_Window*window = createWindow("ACG 2020", (int)size.x, (int)size.y, fullscreen );
	if (!window)
		return 0;
	int window_width, window_height;
	SDL_GetWindowSize(window, &window_width, &window_height);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	const char* glsl_version = "#version 130";
	ImGui_ImplSDL2_InitForOpenGL(window, glcontext);
	ImGui_ImplOpenGL3_Init(glsl_version);

	Input::init(window);

	//launch the game (game is a global variable)
	game = new Application(window_width, window_height, window);

	//main loop, application gets inside here till user closes it
	mainLoop(window);

	//save state and free memory
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
