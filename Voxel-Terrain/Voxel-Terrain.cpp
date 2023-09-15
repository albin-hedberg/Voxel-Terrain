#define SDL_MAIN_HANDLED	// Because fucking SDL.h redefines main() and fucks with compilation! >:(		-> "SDL2main.lib(SDL_windows_main.obj) : error LNK2019..."
// "Redefine main() on some platforms so that it is called by SDL."

#include <SDL.h>
//#undef main				// Possible alternative solution to: #define SDL_MAIN_HANDLED	(ta bort)
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <math.h>
#include <glad/glad.h>

#define WINDOW_W 800	// Display resolution 1280
#define WINDOW_H 600	// Display resolution 720

#define SCALE_FACTOR 200	// Used for rendering Projection
#define CAM_POS_X 512		// Camera starting pos
#define CAM_POS_Y 512
#define RENDER_DISTANCE 2000
#define CAM_HEIGHT 150
#define CAM_HORIZON 200
#define PI 3.141592

#define MAP_N 1024		// Map resolution (1024x1024)
#define MAP_SHIFT 2		// 2 images / Map (Color & Height)

#define FPS 60
#define FRAME_TARGET_TIME (1000 / FPS)	// In miliseconds how long is each frame going to take: 1000/FPS

/***************************************************************
* GLOBALS
****************************************************************/
SDL_Window* window = nullptr;
//SDL_Renderer* renderer = nullptr;
SDL_GLContext glContext = nullptr;
std::string title = "Voxel Terrain";
bool run = false;
int lastFrameTime = 0;
float delta_time = 0;
Uint32 timer = 0;
float counter = 0;
Uint8 currentMap = 0;
bool lbPressed = false;
bool rbPressed = false;
bool walk = false;
bool loopMaps = false;
bool changeMap = false;
Uint8 speed = 2;

/***************************************************************
* BUFFERS FOR MAPS
****************************************************************/
Uint8* heightmap = NULL;	// Buffer/array to hold height values (1024/1024)
Uint8* colormap = NULL;	// Buffer/array to hold color values  (1024/1024)
SDL_Surface* mapColor;
SDL_Surface* mapHeight;
struct
{
	Uint8 r = 0;
	Uint8 g = 0;
	Uint8 b = 0;
} pixelColor;	  // r, g, b = 0

/***************************************************************
* CAMERA
****************************************************************/
struct
{
	float x = CAM_POS_X;	// x position on the map
	float y = CAM_POS_Y;	// y position on the map
	float z = RENDER_DISTANCE;	// distance of the camera looking forward
	float h = CAM_HEIGHT;	// Height of the camera
	float angle = 1.5 * PI;	// Camera angle (radians, clockwise) // The same as 270�
	float horizon = CAM_HORIZON;	// Offset of the horizon position (looking up-down)
	float tilt = 0;
} camera;

/***************************************************************
* INITIALIZATION
****************************************************************/
bool Init_SDL()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "Error initializing SDL." << std::endl;
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	/*
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);	// Disables deprecated OpenGL functions (glStart(); glEnd(); etc...?)
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	*/
	window = SDL_CreateWindow(
		title.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_W,
		WINDOW_H,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);

	if (!window)
	{
		std::cerr << "Error creating SDL Window." << std::endl;
		return false;
	}
	/*
	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer)
	{
		std::cerr << "Error creating SDL Renderer." << std::endl;
		return false;
	}
	*/
	//////////////////////////////////////////////////////////////////////////////
	//// OpenGL Initialization 
	//////////////////////////////////////////////////////////////////////////////
	glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr)
	{
		std::cerr << "Error initializing OpenGL context." << std::endl;
		return false;
	}

	//Initialize the Glad Library
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
	{
		std::cerr << "Error initializing Glad Library." << std::endl;
		return false;
	}

	GLenum error = GL_NO_ERROR;

	//Initialize Projection Matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, WINDOW_W, WINDOW_H, 0.0, 1.0, -1.0); // Change OpenGL coordinates from relative (-1.0 to 1.0 -> 0px to WINDOW_W/H.px)
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cerr << "Error initializing OpenGL Projection Matrix." << std::endl;
		return false;
	}

	//Initialize Modelview Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cerr << "Error initializing OpenGL Modelview Matrix." << std::endl;
		return false;
	}

	//Initialize clear color
	glClearColor(0.4f, 0.6f, 1.f, 1.f);
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cerr << "Error initializing OpenGL clear color." << std::endl;
		return false;
	}
	//////////////////////////////////////////////////////////////////////////////

	// SDL_image init
	int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		std::cerr << "SDL_image could not initialize!" << std::endl;
		return false;
	}

	// Trap mouse
	SDL_SetRelativeMouseMode(SDL_TRUE);

	return true;
}

/***************************************************************
* WINDOW / TITLE
****************************************************************/
void UpdateTitle()
{
	std::string str1 = " - Map: " + std::to_string(currentMap + 1);

	std::string str2 = "";
	if (speed == 1)
		str2 = " - Speed: Slow";
	else if (speed == 2)
		str2 = " - Speed: Normal";
	else if (speed == 3)
		str2 = " - Speed: Fast";


	std::string str3 = "";
	if (loopMaps)
		str3 = " - Next map: " + std::to_string(10 - (timer % 10)) + " sec.";
	else
		str3 = "";

	std::string finalStr = title + str1 + str2 + str3;

	//std::string finalStr = title + str1 + str2;
	SDL_SetWindowTitle(window, finalStr.c_str());
}

/***************************************************************
* MAPS
****************************************************************/
const char* MAPS[62] = {
	"gif/map0.color.gif",
	"gif/map0.height.gif",

	"gif/map1.color.gif",
	"gif/map1.height.gif",

	"gif/map2.color.gif",
	"gif/map2.height.gif",

	"gif/map3.color.gif",
	"gif/map3.height.gif",

	"gif/map4.color.gif",
	"gif/map4.height.gif",

	"gif/map5.color.gif",
	"gif/map5.height.gif",

	"gif/map6.color.gif",
	"gif/map6.height.gif",

	"gif/map7.color.gif",
	"gif/map7.height.gif",

	"gif/map8.color.gif",
	"gif/map8.height.gif",

	"gif/map9.color.gif",
	"gif/map9.height.gif",

	"gif/map10.color.gif",
	"gif/map10.height.gif",

	"gif/map11.color.gif",
	"gif/map11.height.gif",

	"gif/map12.color.gif",
	"gif/map12.height.gif",

	"gif/map13.color.gif",
	"gif/map13.height.gif",

	"gif/map14.color.gif",
	"gif/map14.height.gif",

	"gif/map15.color.gif",
	"gif/map15.height.gif",

	"gif/map16.color.gif",
	"gif/map16.height.gif",

	"gif/map17.color.gif",
	"gif/map17.height.gif",

	"gif/map18.color.gif",
	"gif/map18.height.gif",

	"gif/map19.color.gif",
	"gif/map19.height.gif",

	"gif/map20.color.gif",
	"gif/map20.height.gif",

	"gif/map21.color.gif",
	"gif/map21.height.gif",

	"gif/map22.color.gif",
	"gif/map22.height.gif",

	"gif/map23.color.gif",
	"gif/map23.height.gif",

	"gif/map24.color.gif",
	"gif/map24.height.gif",

	"gif/map25.color.gif",
	"gif/map25.height.gif",

	"gif/map26.color.gif",
	"gif/map26.height.gif",

	"gif/map27.color.gif",
	"gif/map27.height.gif",

	"gif/map28.color.gif",
	"gif/map28.height.gif",

	"gif/map29.color.gif",
	"gif/map29.height.gif",

	"gif/mapDF.detail.gif",
	"gif/mapDF.height.gif"
};
void LoadMap(int index)
{
	mapColor = IMG_Load(MAPS[index]);
	if (!mapColor)
	{
		std::cerr << "Failed to load color image!" << std::endl;
		//run = false;
	}
	mapHeight = IMG_Load(MAPS[index + 1]);
	if (!mapHeight)
	{
		std::cerr << "Failed to load height image!" << std::endl;
		//run = false;
	}

	SDL_LockSurface(mapColor);
	colormap = (Uint8*)mapColor->pixels;
	SDL_UnlockSurface(mapColor);

	SDL_LockSurface(mapHeight);
	heightmap = (Uint8*)mapHeight->pixels;
	SDL_UnlockSurface(mapHeight);

	UpdateTitle();
}

/***************************************************************
* INPUT
****************************************************************/
void Input()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
			run = false;
		if (event.type == SDL_MOUSEMOTION)
		{
			if (event.motion.xrel) // Look left/right
			{
				camera.angle += event.motion.xrel * 0.01 * delta_time * 0.01;
			}

			if (event.motion.yrel < 0)	// Look up
			{
				camera.horizon += 2 * delta_time * 0.03;
				if (camera.horizon > 700)
					camera.horizon = 700;
			}
			if (event.motion.yrel > 0)	// Look down
			{
				camera.horizon -= 2 * delta_time * 0.03;
				if (camera.horizon < -100)
					camera.horizon = -100;
			}
		}

		if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				lbPressed = true;
			}
			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				rbPressed = true;
			}
		}

		if (event.type == SDL_MOUSEBUTTONUP)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				lbPressed = false;
			}
			if (event.button.button == SDL_BUTTON_RIGHT)
			{
				rbPressed = false;
			}
			if (event.button.button == SDL_BUTTON_X2) // Map +
			{
				currentMap += 1;

				if (currentMap > 30)
					currentMap = 0;

				LoadMap(currentMap * MAP_SHIFT);
			}
			else if (event.button.button == SDL_BUTTON_X1) // Map -
			{
				currentMap -= 1;

				if (currentMap > 30)
					currentMap = 30;

				LoadMap(currentMap * MAP_SHIFT);
			}
		}

		if (event.type == SDL_MOUSEWHEEL)
		{
			if (event.wheel.y > 0) // scroll up
			{
				if (speed > 2)
				{
					speed = 3;
				}
				else
				{
					speed += 1;
					UpdateTitle();
				}
			}
			else if (event.wheel.y < 0) // scroll down
			{
				if (speed < 2)
				{
					speed = 1;
				}
				else
				{
					speed -= 1;
					UpdateTitle();
				}
			}
		}
		if (event.type == SDL_KEYUP)
		{
			if (event.key.keysym.scancode == SDL_SCANCODE_Z || event.key.keysym.scancode == SDL_SCANCODE_END) // Change map -
			{
				currentMap -= 1;

				if (currentMap > 30)
					currentMap = 30;

				LoadMap(currentMap * MAP_SHIFT);
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_X || event.key.keysym.scancode == SDL_SCANCODE_HOME) // Change map +
			{
				currentMap += 1;

				if (currentMap > 30)
					currentMap = 0;

				LoadMap(currentMap * MAP_SHIFT);
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_RETURN)
			{
				if (loopMaps)
				{
					loopMaps = false;
				}
				else
				{
					loopMaps = true;
				}

				UpdateTitle();
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_RCTRL)
			{
				if (walk)
				{
					walk = false;
				}
				else
				{
					walk = true;
				}
			}
		}
	}

	const Uint8* keystates = SDL_GetKeyboardState(NULL);

	if (keystates[SDL_SCANCODE_ESCAPE])
	{
		run = false;
	}
	if (keystates[SDL_SCANCODE_W] || keystates[SDL_SCANCODE_UP]) // Move forward
	{
		if (speed == 1)
		{
			camera.x += cos(camera.angle) * delta_time * 0.1 / 4;
			camera.y += sin(camera.angle) * delta_time * 0.1 / 4;
		}
		else if (speed == 2)
		{
			camera.x += cos(camera.angle) * delta_time * 0.1;
			camera.y += sin(camera.angle) * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.x += cos(camera.angle) * delta_time * 0.1 * 4;
			camera.y += sin(camera.angle) * delta_time * 0.1 * 4;
		}

		//std::cout << "X: " << camera.x << "\tY: " << camera.y << "\n";
	}
	if (keystates[SDL_SCANCODE_S] || keystates[SDL_SCANCODE_DOWN]) // Move backward 
	{
		if (speed == 1)
		{
			camera.x -= cos(camera.angle) * delta_time * 0.1 / 4;
			camera.y -= sin(camera.angle) * delta_time * 0.1 / 4;
		}
		else if (speed == 2)
		{
			camera.x -= cos(camera.angle) * delta_time * 0.1;
			camera.y -= sin(camera.angle) * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.x -= cos(camera.angle) * delta_time * 0.1 * 4;
			camera.y -= sin(camera.angle) * delta_time * 0.1 * 4;
		}
	}
	if (keystates[SDL_SCANCODE_A] || keystates[SDL_SCANCODE_LEFT]) // Move Left 
	{
		if (speed == 1)
		{
			camera.x -= cos(camera.angle + 1.6) * delta_time * 0.1 / 4;
			camera.y -= sin(camera.angle + 1.6) * delta_time * 0.1 / 4;
		}
		else if (speed == 2)
		{
			camera.x -= cos(camera.angle + 1.6) * delta_time * 0.1;
			camera.y -= sin(camera.angle + 1.6) * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.x -= cos(camera.angle + 1.6) * delta_time * 0.1 * 4;
			camera.y -= sin(camera.angle + 1.6) * delta_time * 0.1 * 4;
		}
		//camera.angle -= 0.1; // Turn left
	}
	if (keystates[SDL_SCANCODE_D] || keystates[SDL_SCANCODE_RIGHT]) // Move Right 
	{
		if (speed == 1)
		{
			camera.x += cos(camera.angle + 1.6) * delta_time * 0.1 / 4;
			camera.y += sin(camera.angle + 1.6) * delta_time * 0.1 / 4;
		}
		else if (speed == 2)
		{
			camera.x += cos(camera.angle + 1.6) * delta_time * 0.1;
			camera.y += sin(camera.angle + 1.6) * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.x += cos(camera.angle + 1.6) * delta_time * 0.1 * 4;
			camera.y += sin(camera.angle + 1.6) * delta_time * 0.1 * 4;
		}
		//camera.angle += 0.1; // Turn right
	}
	if (keystates[SDL_SCANCODE_E]) // Tilt up
	{
		camera.horizon += 5;
	}
	if (keystates[SDL_SCANCODE_Q]) // Tilt down
	{
		camera.horizon -= 5;
	}
	if (keystates[SDL_SCANCODE_R] || keystates[SDL_SCANCODE_PAGEUP]) // Move up
	{
		if (speed == 1)
		{
			camera.h += 2 * delta_time * 0.1 / 4;
		}
		else if (speed == 2)
		{
			camera.h += 2 * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.h += 2 * delta_time * 0.1 * 4;
		}
	}
	if (keystates[SDL_SCANCODE_F] || keystates[SDL_SCANCODE_PAGEDOWN]) // Move down
	{
		if (speed == 1)
		{
			camera.h -= 2 * delta_time * 0.1 / 4;
		}
		else if (speed == 2)
		{
			camera.h -= 2 * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.h -= 2 * delta_time * 0.1 * 4;
		}
	}
	if (keystates[SDL_SCANCODE_KP_PLUS] || keystates[SDL_SCANCODE_INSERT]) // Increase render distance
	{
		if (camera.z < 10000)
			camera.z += 25;
	}
	if (keystates[SDL_SCANCODE_KP_MINUS] || keystates[SDL_SCANCODE_DELETE]) // Decrease render distance
	{
		if (camera.z > 125)
			camera.z -= 25;
	}
	if (keystates[SDL_SCANCODE_SPACE])	// Reset camera
	{
		camera.x = CAM_POS_X;
		camera.y = CAM_POS_Y;
		camera.z = RENDER_DISTANCE;
		camera.h = CAM_HEIGHT;
		camera.angle = 1.5 * PI;
		camera.horizon = CAM_HORIZON;
		speed = 2;
		UpdateTitle();
	}

	if (lbPressed)
	{
		if (speed == 1)
		{
			camera.h -= 2 * delta_time * 0.1 / 2;
		}
		else if (speed == 2)
		{
			camera.h -= 2 * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.h -= 2 * delta_time * 0.1 * 2;
		}
	}
	if (rbPressed)
	{
		if (speed == 1)
		{
			camera.h += 2 * delta_time * 0.1 / 2;
		}
		else if (speed == 2)
		{
			camera.h += 2 * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.h += 2 * delta_time * 0.1 * 2;
		}
	}
}

/***************************************************************
* UPDATE
****************************************************************/
void Update()
{
	delta_time = (SDL_GetTicks() - lastFrameTime);
	// Comment out if you don't want to cap the fps
	// Calculate how much we have to wait until we reach the target frame time
	int timeToWait = FRAME_TARGET_TIME - delta_time;

	// Only Delay if we are too fast too update this frame
	if (timeToWait > 0 && timeToWait <= FRAME_TARGET_TIME)
	{
		SDL_Delay(timeToWait);	// Sleep until we reach the frame target time
	}

	// Get delta_time factor converted to seconds to be used to update objects
	//float delta_time = (SDL_GetTicks() - lastFrameTime) / 1000.0F;

	if (loopMaps)
	{
		timer = (int)(SDL_GetTicks() / 1000);

		if ((timer % 10) == 0)	// Change map every 5 seconds
		{
			if (!changeMap)
			{
				changeMap = true;
				currentMap += 1;

				if (currentMap > 30)
					currentMap = 0;

				LoadMap(currentMap * MAP_SHIFT);
			}
		}
		else
		{
			changeMap = false;
			UpdateTitle();
		}
	}

	// Store milliseconds of the current frame to be used in the next one
	lastFrameTime = SDL_GetTicks();
	// (int)(1000.0F / (FRAME_TARGET_TIME - delta_time)) <- FPS
}

/***************************************************************
* RENDERING
****************************************************************/
/*
void Render()
{
	//SDL_SetRenderDrawColor(renderer, 25, 75, 150, 255);	// Blue sky
	//SDL_RenderClear(renderer);
	//SDL_SetRenderDrawColor(renderer, 75, 0, 0, 255);
	//SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);

	float sinAngle = sin(camera.angle);
	float cosAngle = cos(camera.angle);

	// Left-most point of the FOV
	float plx = cosAngle * camera.z + sinAngle * camera.z;	// pl = Pont Left
	float ply = sinAngle * camera.z - cosAngle * camera.z;	// originally: +camera.z

	// Right-most point of the FOV
	float prx = cosAngle * camera.z - sinAngle * camera.z;	// pr = Pont Right
	float pry = sinAngle * camera.z + cosAngle * camera.z;

	// Loop WINDOW_W rays from left to right
	for (int x = 0; x < WINDOW_W; x++)
	{
		float delta_x = (plx + (prx - plx) / WINDOW_W * x) / camera.z;
		float delta_y = (ply + (pry - ply) / WINDOW_W * x) / camera.z;

		// Ray (x,y) coords
		float rayX = camera.x;
		float rayY = camera.y;

		// Collision detection
		int collisionOffset = ((MAP_N * ((int)(rayY) & (MAP_N - 1))) + ((int)(rayX) & (MAP_N - 1)));
		// camera.h = ((int)heightmap[collisionOffset] + 10); // walk
		if (((int)heightmap[collisionOffset] + 10) > camera.h)
		{
			camera.h = ((int)heightmap[collisionOffset] + 10);
		}

		// Store the tallest projected height per-ray
		float maxHeight = WINDOW_H;

		// Loop all depth units until the zfar distance limit
		for (int z = 1; z < camera.z; z++)
		{
			rayX += delta_x;
			rayY += delta_y;

			// Find the offset that we have to go and fetch values from the heightmap
			//int mapOffset = (int)((1024 * ((int)rayY & 1023)) + ((int)rayX & 1023));	// Bitwise '&' clamps the value between 0 - 1023 = infinite
			int mapOffset = ((MAP_N * ((int)(rayY) & (MAP_N - 1))) + ((int)(rayX) & (MAP_N - 1)));

			// Project height values and find the height on-screen	// Projection: (Pos - Offset / z * Scale factor + horizon offset
			int heightOnScreen = (int)((camera.h - heightmap[mapOffset]) / z * SCALE_FACTOR + camera.horizon);

			// Only draw pixels if the new projected height is taller than the previous tallest height (maxHeight)
			if (heightOnScreen < maxHeight)
			{
				// Draw pixels from previous maxHeight until the new projected height
				for (int y = heightOnScreen; y < maxHeight; y++)
				{
					// draw		      width * y = row, i = x -> screen coordinates
					//framebuffer[(WINDOW_W * y) + i] = (uint8_t)colormap[mapOffset];
					//currentPixel = GetPixelColor(mapColor, rayX, rayY);

					SDL_GetRGB(colormap[mapOffset], mapColor->format, &pixelColor.r, &pixelColor.g, &pixelColor.b);
					//SDL_SetRenderDrawColor(renderer, pixelColor.r, pixelColor.g, pixelColor.b, SDL_ALPHA_OPAQUE);
					//SDL_RenderDrawPoint(renderer, x, y);
				}
				maxHeight = heightOnScreen;
			}
		}
	}
	//SDL_RenderPresent(renderer);
}
*/
void RenderGL()
{
	glClear(GL_COLOR_BUFFER_BIT);

	float sinAngle = sin(camera.angle);
	float cosAngle = cos(camera.angle);

	// Left-most point of the FOV
	float plx = cosAngle * camera.z + sinAngle * camera.z;	// pl = Pont Left
	float ply = sinAngle * camera.z - cosAngle * camera.z;

	// Right-most point of the FOV
	float prx = cosAngle * camera.z - sinAngle * camera.z;	// pr = Pont Right
	float pry = sinAngle * camera.z + cosAngle * camera.z;

	// Loop WINDOW_W rays from left to right
	for (int x = 0; x < WINDOW_W; x++)
	{
		float delta_x = (plx + (prx - plx) / WINDOW_W * x) / camera.z;
		float delta_y = (ply + (pry - ply) / WINDOW_W * x) / camera.z;

		// Ray (x,y) coords
		float rayX = camera.x;
		float rayY = camera.y;

		// Collision detection
		int collisionOffset = ((MAP_N * ((int)(rayY) & (MAP_N - 1))) + ((int)(rayX) & (MAP_N - 1)));

		if (walk)
			camera.h = ((int)heightmap[collisionOffset] + 10); // lock camera to ground/walk

		if (camera.h < ((int)heightmap[collisionOffset] + 10))
		{
			camera.h = ((int)heightmap[collisionOffset] + 10);
		}

		if (loopMaps)
		{
			int collisionFront = ((MAP_N * ((int)(rayY + (sin(camera.angle) * 25)) & (MAP_N - 1))) + ((int)(rayX + (cos(camera.angle) * 25)) & (MAP_N - 1)));	// Height in Front of camera

			if (camera.h < ((int)heightmap[collisionFront] + 10))
			{
				int collisionRight = ((MAP_N * ((int)(rayY + (sin(camera.angle + 1) * 50)) & (MAP_N - 1))) + ((int)(rayX + (cos(camera.angle + 1) * 50)) & (MAP_N - 1)));	// Height to Right (angle + 1)
				int collisionLeft = ((MAP_N * ((int)(rayY + (sin(camera.angle - 1) * 50)) & (MAP_N - 1))) + ((int)(rayX + (cos(camera.angle - 1) * 50)) & (MAP_N - 1)));	// Height to Left  (angle - 1)

				if (camera.h > ((int)heightmap[collisionRight]) && camera.h < ((int)heightmap[collisionLeft]))	// Height lower to the Right
					camera.angle += 0.000005 * delta_time;	// Turn Right
				if (camera.h < ((int)heightmap[collisionRight]) && camera.h >((int)heightmap[collisionLeft]))	// Height lower to the Left
					camera.angle -= 0.000005 * delta_time;	// Turn Left
			}
		}

		// Store the tallest projected height per-ray
		float maxHeight = WINDOW_H + 1;

		// Loop all depth units until the zfar distance limit
		for (int z = 1; z < camera.z; z++)
		{
			rayX += delta_x;
			rayY += delta_y;

			// Find the offset that we have to go and fetch values from the heightmap - Bitwise '&' clamps the value between 0 - 1023 = world repeats
			int mapOffset = ((MAP_N * ((int)(rayY) & (MAP_N - 1))) + ((int)(rayX) & (MAP_N - 1)));

			// Project height values and find the height on-screen	// Projection: (Pos - Offset / z * Scale factor + horizon offset
			int heightOnScreen = (int)((camera.h - heightmap[mapOffset]) / z * SCALE_FACTOR + camera.horizon);

			// Only draw pixels if the new projected height is taller than the previous tallest height (maxHeight)
			if (heightOnScreen < maxHeight)
			{
				if (loopMaps)
				{
					float lean = (camera.tilt * (x / (float)WINDOW_W - 0.5) + 0.5) * WINDOW_H / 3;

					for (int y = (heightOnScreen + lean); y < (maxHeight + lean); y++)
					{
						if (y >= 0)
						{
							SDL_GetRGB(colormap[mapOffset], mapColor->format, &pixelColor.r, &pixelColor.g, &pixelColor.b);

							glBegin(GL_POINTS);
							glColor3ub(pixelColor.r, pixelColor.g, pixelColor.b);
							glVertex2i(x, y);
							glEnd();
						}
					}
				}
				else
				{
					// Draw pixels from previous maxHeight until the new projected height
					for (int y = heightOnScreen; y < maxHeight; y++)
					{
						SDL_GetRGB(colormap[mapOffset], mapColor->format, &pixelColor.r, &pixelColor.g, &pixelColor.b);

						glBegin(GL_POINTS);
						glColor3ub(pixelColor.r, pixelColor.g, pixelColor.b);
						glVertex2i(x, y);
						glEnd();
					}
				}
				maxHeight = heightOnScreen;
			}
		}
	}
	/*
	glBegin(GL_LINES);
		glColor3ub(0, 255, 0);
		// Top
		glVertex2i(WINDOW_W / 2, WINDOW_H / 2 - 5);
		glVertex2i(WINDOW_W / 2, WINDOW_H / 2 - 15);
		// Bottom
		glVertex2i(WINDOW_W / 2, WINDOW_H / 2 + 5);
		glVertex2i(WINDOW_W / 2, WINDOW_H / 2 + 15);
		// Left
		glVertex2i(WINDOW_W / 2 - 5, WINDOW_H / 2);
		glVertex2i(WINDOW_W / 2 - 15, WINDOW_H / 2);
		// Right
		glVertex2i(WINDOW_W / 2 + 5, WINDOW_H / 2);
		glVertex2i(WINDOW_W / 2 + 15, WINDOW_H / 2);
	glEnd();

	for (int mX = 0; mX < 1024; mX++)
	{
		for (int mY = 0; mY < 1024; mY++)
		{
			int mMapOffset = (1024 * mY + mX);
			SDL_GetRGB(colormap[mMapOffset], mapColor->format, &pixelColor.r, &pixelColor.g, &pixelColor.b);
			glBegin(GL_POINTS);
				glColor3ub(pixelColor.r, pixelColor.g, pixelColor.b);
				glVertex2i(mX/6, mY/6);
			glEnd();
		}
	}
	*/
	SDL_GL_SwapWindow(window);
}
/*
void RenderGL2()
{
	glClear(GL_COLOR_BUFFER_BIT);

	float sinAngle = sin(camera.angle);
	float cosAngle = cos(camera.angle);

	float maxHeight = WINDOW_H;

	float delta_z = 1.0F;

	for (int z = 1; z < camera.z; z++)
	{
		float plx = -cosAngle * delta_z - sinAngle * delta_z;
		float ply =  sinAngle * delta_z - cosAngle * delta_z;
		float prx =  cosAngle * delta_z - sinAngle * delta_z;
		float pry = -sinAngle * delta_z - cosAngle * delta_z;

		float delta_x = (prx - plx) / WINDOW_W;
		float delta_y = (pry - ply) / WINDOW_W;

		plx += camera.x;
		ply += camera.y;

		float invz = 1.0F / delta_z * 240;

		for (int x = 0; x < WINDOW_W; x++)
		{
			int mapOffset = ((((int)(ply) & (MAP_N - 1)) << 10) + ((int)(plx) & (MAP_N - 1)));
			int heightOnScreen = (int)((camera.h - heightmap[mapOffset]) * invz + camera.horizon);

			if (heightOnScreen < 0)
			{
				heightOnScreen = 0;
			}

			if (heightOnScreen > maxHeight)
			{
				return;
			}

			//int offset = ((heightOnScreen * WINDOW_H) + x);
			for (int y = heightOnScreen; y < maxHeight; y++)
			{
				SDL_GetRGB(colormap[mapOffset], mapColor->format, &pixelColor.r, &pixelColor.g, &pixelColor.b);

				glBegin(GL_POINTS);
				glColor3ub(pixelColor.r, pixelColor.g, pixelColor.b);
				glVertex2i(x, y);
				glEnd();
			}

			if (heightOnScreen < maxHeight)
			{
				maxHeight = heightOnScreen;
			}

			plx += delta_x;
			ply += delta_y;
		}
		delta_z += 0.005;
	}

	SDL_GL_SwapWindow(window);
}
*/
/***************************************************************
* DEMO
****************************************************************/
void Demo()
{
	if (loopMaps)
	{
		if (speed == 1)
		{
			camera.h = (sin(counter) * 100) + 175;	// Up/Down movement
			if (walk)
				camera.horizon = CAM_HORIZON;		// Horizon
			else
				camera.horizon = -1 * ((sin(counter - 1) * 100) + 200) + 200;
			if ((20 - (timer % 20)) < 5)
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0002;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			else if ((20 - (timer % 20)) < 15)
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0003;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			else
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0001;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			camera.x += cos(camera.angle) * delta_time * 0.1 / 2;
			camera.y += sin(camera.angle) * delta_time * 0.1 / 2;
		}
		else if (speed == 2)
		{
			camera.h = (sin(counter) * 100) + 175;	// Up/Down movement
			if (walk)
				camera.horizon = CAM_HORIZON;		// Horizon
			else
				camera.horizon = -1 * ((sin(counter - 1) * 100) + 200) + 200;
			if ((20 - (timer % 20)) < 5)
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0002;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			else if ((20 - (timer % 20)) < 15)
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0003;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			else
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0001;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			camera.x += cos(camera.angle) * delta_time * 0.1;
			camera.y += sin(camera.angle) * delta_time * 0.1;
		}
		else if (speed == 3)
		{
			camera.h = (sin(counter) * 100) + 175;	// Up/Down movement
			if (walk)
				camera.horizon = CAM_HORIZON;			// Horizon
			else
				camera.horizon = -1 * ((sin(counter - 1) * 100) + 200) + 200;
			if ((20 - (timer % 20)) < 5)
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0002;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			else if ((20 - (timer % 20)) < 15)
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0003;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			else
			{
				camera.angle += (sin(counter) * 2) * delta_time * 0.0001;
				camera.tilt = -1 * (sin(counter) * 0.3);
			}
			camera.x += cos(camera.angle) * delta_time * 0.1 * 2;
			camera.y += sin(camera.angle) * delta_time * 0.1 * 2;
		}

		counter += 0.02;

		//std::cout << " tilt: " << (float)camera.tilt << "\n";
		/*
		std::cout << " Hz: " << (int)camera.horizon;
		std::cout << " H: " << (int)camera.h;
		std::cout << " X: " << (int)camera.x;
		std::cout << " Y: " << (int)camera.y;
		std::cout << " A: " << (float)camera.angle;
		std::cout << "\n";
		*/
	}
}

/***************************************************************
* QUIT
****************************************************************/
void Cleanup()
{
	SDL_FreeSurface(mapColor);
	SDL_FreeSurface(mapHeight);

	SDL_GL_DeleteContext(glContext);
	//SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}

/***************************************************************
* MAIN
****************************************************************/
int main(int argc, char* argv[])
{
	run = Init_SDL();

	LoadMap(currentMap * MAP_SHIFT);

	// DEBUG TESTING
	//currentPixel = GetPixelColor(mapColor, 0, 0);
	//std::cout << "R: " << (int)currentPixel.r << " G: " << (int)currentPixel.g << " B: " << (int)currentPixel.b << " A: " << (int)currentPixel.a << "\n";
	//std::cout << mapHeight->w << std::endl;
	/*
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, mapColor);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	*/

	std::cout << "-Keyboard-\n";
	std::cout << "W, A, S, D, < ^ v >:\tMove\n";
	std::cout << "R, F, PgUp, PgDn:\tMove up/down\n";
	std::cout << "Num +/-, Ins, Del:\tRender distance\n";
	std::cout << "Z, X, Home, End:\tChange map\n";
	std::cout << "Q, E:\t\t\tLook up / down\n";
	std::cout << "RCtrl:\t\t\tWalk\n";
	std::cout << "Space:\t\t\tReset\n";
	std::cout << "Enter:\t\t\tDemo\n";
	std::cout << "Escape:\t\t\tQuit\n\n";
	std::cout << "-Mouse-\n";
	std::cout << "LeftClick, RightClick:\tMove up/down\n";
	std::cout << "Side Buttons:\t\tChange map\n";
	std::cout << "Scroll Wheel:\t\tSpeed\n\n";
	std::cout << "Total maps:\t\t30 (+1 debug map)\n";

	while (run)
	{
		Input();
		Update();
		Demo();
		//Render();
		RenderGL();
		//RenderGL2();
	}

	Cleanup();

	return 0;
}

/*
* TODO:
*	+ OpenGL rendering
*	+ Change map (1-30)
*	+ No clipping into ground, camera.h <= height
*	- Render Res vs. Display res
*	+ Fix Input! + mouse (ghosting)
*	- Minimap?
*	+ Camera tilt
*/
