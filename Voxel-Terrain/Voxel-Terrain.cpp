#define SDL_MAIN_HANDLED	// "SDL2main.lib(SDL_windows_main.obj) : error LNK2019..."

#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <math.h>
#include <glad/glad.h>

#define WINDOW_W 800
#define WINDOW_H 600

#define SCALE_FACTOR 200	// Används till rendering projection
#define CAM_POS_X 512
#define CAM_POS_Y 512
#define RENDER_DISTANCE 2000
#define CAM_HEIGHT 150
#define CAM_HORIZON 200
#define PI 3.141592

#define MAP_N 1024		// Map resolution (1024x1024)
#define MAP_SHIFT 2		// 2 bilder / karta (Color & Height)

#define FPS 60
#define FRAME_TARGET_TIME (1000 / FPS)

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
* BUFFERS FÖR KARTOR
****************************************************************/
Uint8* heightmap = NULL;
Uint8* colormap = NULL;
SDL_Surface* mapColor;
SDL_Surface* mapHeight;
struct
{
	Uint8 r = 0;
	Uint8 g = 0;
	Uint8 b = 0;
} pixelColor;

/***************************************************************
* KAMERA
****************************************************************/
struct
{
	float x = CAM_POS_X;
	float y = CAM_POS_Y;
	float z = RENDER_DISTANCE;
	float h = CAM_HEIGHT;
	float angle = 1.5 * PI;
	float horizon = CAM_HORIZON;
	float tilt = 0;
} camera;

/***************************************************************
* INIT
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
	
	//////////////////////////////////////////////////////////////////////////////
	//// OpenGL Init 
	//////////////////////////////////////////////////////////////////////////////
	glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr)
	{
		std::cerr << "Error initializing OpenGL context." << std::endl;
		return false;
	}

	// Init Glad Library
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
	{
		std::cerr << "Error initializing Glad Library." << std::endl;
		return false;
	}

	GLenum error = GL_NO_ERROR;

	// Init Projection Matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, WINDOW_W, WINDOW_H, 0.0, 1.0, -1.0);
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cerr << "Error initializing OpenGL Projection Matrix." << std::endl;
		return false;
	}

	// Init Modelview Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cerr << "Error initializing OpenGL Modelview Matrix." << std::endl;
		return false;
	}

	// Init clear color
	glClearColor(0.4f, 0.6f, 1.f, 1.f);
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cerr << "Error initializing OpenGL clear color." << std::endl;
		return false;
	}
	//////////////////////////////////////////////////////////////////////////////

	// Init SDL_image
	int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		std::cerr << "SDL_image could not initialize!" << std::endl;
		return false;
	}

	// Fånga muspekaren
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
	SDL_SetWindowTitle(window, finalStr.c_str());
}

/***************************************************************
* KARTOR
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
	int timeToWait = FRAME_TARGET_TIME - delta_time;

	if (timeToWait > 0 && timeToWait <= FRAME_TARGET_TIME)
	{
		SDL_Delay(timeToWait);
	}

	if (loopMaps)
	{
		timer = (int)(SDL_GetTicks() / 1000);

		if ((timer % 10) == 0)
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

	lastFrameTime = SDL_GetTicks();
}

/***************************************************************
* RENDERING
****************************************************************/

void RenderGL()
{
	glClear(GL_COLOR_BUFFER_BIT);

	float sinAngle = sin(camera.angle);
	float cosAngle = cos(camera.angle);

	float plx = cosAngle * camera.z + sinAngle * camera.z;	// pl = Point Left
	float ply = sinAngle * camera.z - cosAngle * camera.z;

	float prx = cosAngle * camera.z - sinAngle * camera.z;	// pr = Point Right
	float pry = sinAngle * camera.z + cosAngle * camera.z;

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
			camera.h = ((int)heightmap[collisionOffset] + 10); // Fäst kameran mot marken / walk

		if (camera.h < ((int)heightmap[collisionOffset] + 10))
		{
			camera.h = ((int)heightmap[collisionOffset] + 10);
		}

		if (loopMaps)
		{
			int collisionFront = ((MAP_N * ((int)(rayY + (sin(camera.angle) * 25)) & (MAP_N - 1))) + ((int)(rayX + (cos(camera.angle) * 25)) & (MAP_N - 1)));

			if (camera.h < ((int)heightmap[collisionFront] + 10))
			{
				int collisionRight = ((MAP_N * ((int)(rayY + (sin(camera.angle + 1) * 50)) & (MAP_N - 1))) + ((int)(rayX + (cos(camera.angle + 1) * 50)) & (MAP_N - 1)));	// Höjd till höger (angle + 1)
				int collisionLeft = ((MAP_N * ((int)(rayY + (sin(camera.angle - 1) * 50)) & (MAP_N - 1))) + ((int)(rayX + (cos(camera.angle - 1) * 50)) & (MAP_N - 1)));	// Höjd till vänster  (angle - 1)

				if (camera.h > ((int)heightmap[collisionRight]) && camera.h < ((int)heightmap[collisionLeft]))
					camera.angle += 0.000005 * delta_time;
				if (camera.h < ((int)heightmap[collisionRight]) && camera.h >((int)heightmap[collisionLeft]))
					camera.angle -= 0.000005 * delta_time;
			}
		}

		float maxHeight = WINDOW_H + 1;

		for (int z = 1; z < camera.z; z++)
		{
			rayX += delta_x;
			rayY += delta_y;

			// Offset för att hämta rätt värden från heightmap, bitwise '&' håller värdet mellan 0 - 1023 = kartan upprepas
			int mapOffset = ((MAP_N * ((int)(rayY) & (MAP_N - 1))) + ((int)(rayX) & (MAP_N - 1)));
			
			int heightOnScreen = (int)((camera.h - heightmap[mapOffset]) / z * SCALE_FACTOR + camera.horizon);

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
	
	SDL_GL_SwapWindow(window);
}

/***************************************************************
* DEMO
****************************************************************/
void Demo()
{
	if (loopMaps)
	{
		if (speed == 1)
		{
			camera.h = (sin(counter) * 100) + 175;
			if (walk)
				camera.horizon = CAM_HORIZON;
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
			camera.h = (sin(counter) * 100) + 175;
			if (walk)
				camera.horizon = CAM_HORIZON;
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
			camera.h = (sin(counter) * 100) + 175;
			if (walk)
				camera.horizon = CAM_HORIZON;
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
		RenderGL();
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
