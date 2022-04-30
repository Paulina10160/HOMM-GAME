#include <stdio.h>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_image.h"
#include <stdbool.h>

// Structure that contains data about the object texture, position, speed and its size
struct ObjectData {
	SDL_Texture* texture;
	float positionX;
	float positionY;
	int width;
	int height;
	float speed;
	int currentCellX;
	int currentCellY;
};
typedef struct ObjectData ObjectData; // Creating shortcut for the struct that is declared above

#define CELLS_X 15
#define CELLS_Y 11

struct Board
{
	SDL_Texture* defaultCellTexture;
	SDL_Texture* obstacleCellTexture;
	unsigned char cells[CELLS_Y][CELLS_X];
	unsigned char cellsOld[CELLS_Y][CELLS_X];
};
typedef struct Board Board;

#define CELL_SIZE 50

// Global variables that contains window size 
int WINDOW_WIDTH = 15 * CELL_SIZE;
int WINDOW_HEIGHT = 11 * CELL_SIZE;

void initCells(Board* board);
void findNextCellDestiny(ObjectData* car, unsigned char cells[CELLS_Y][CELLS_X], int* cellTargetX, int* cellTargetY, int* carDestinyX, int* carDestinyY);

int main()
{
	// Init SDL libraries
	SDL_SetMainReady(); // Just leave it be
	int result = 0;
	result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); // Init of the main SDL library
	if (result) // SDL_Init returns 0 (false) when everything is OK
	{
		printf("Can't initialize SDL. Error: %s", SDL_GetError()); // SDL_GetError() returns a string (as const char*) which explains what went wrong with the last operation
		return -1;
	}

	result = IMG_Init(IMG_INIT_PNG); // Init of the Image SDL library. We only need to support PNG for this project
	if (!(result & IMG_INIT_PNG)) // Checking if the PNG decoder has started successfully
	{
		printf("Can't initialize SDL image. Error: %s", SDL_GetError());
		return -1;
	}

	// Creating the window 1920x1080 (could be any other size)
	SDL_Window* window = SDL_CreateWindow("FirstSDL",
		0, 0,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN);

	if (!window)
		return -1;

	// Setting the window to fullscreen 
	//if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) < 0) {
	//	printf("Could not set up the fullscreen\n");
	//	return -1;
	//}

	// Creating a renderer which will draw things on the screen
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer)
		return -1;

	// Setting the color of an empty window (RGBA). You are free to adjust it.
	SDL_SetRenderDrawColor(renderer, 20, 150, 39, 255);

	// Creating the game object (car)
	ObjectData car;
	// Here the surface is the information about the image. It contains the color data, width, height and other info.
	SDL_Surface* surface = IMG_Load("image.png");
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", "image.png", IMG_GetError());
		return -1;
	}

	// Now we use the renderer and the surface to create a texture which we later can draw on the screen.
	SDL_Texture* carTexture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!carTexture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return -1;
	}

	// Bye-bye the surface
	SDL_FreeSurface(surface);
	car.texture = carTexture;
	car.positionX = 25.f;
	car.positionY = 25.f;
	car.width = CELL_SIZE;
	car.height = CELL_SIZE;
	car.speed = 200.f;
	car.currentCellX = 0;
	car.currentCellY = 0;

	Board board;
	initCells(&board);
	surface = IMG_Load("cellDefault.png");
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", "cellDefault.png", IMG_GetError());
		return -1;
	}

	// Now we use the renderer and the surface to create a texture which we later can draw on the screen.
	SDL_Texture* defaultCellTexture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!defaultCellTexture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return -1;
	}
	board.defaultCellTexture = defaultCellTexture;
	// Bye-bye the surface
	SDL_FreeSurface(surface);

	surface = IMG_Load("cellObstacle.png");
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", "cellObstacle.png", IMG_GetError());
		return -1;
	}

	// Now we use the renderer and the surface to create a texture which we later can draw on the screen.
	SDL_Texture* obstacleCellTexture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!obstacleCellTexture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return -1;
	}
	board.obstacleCellTexture = obstacleCellTexture;
	// Bye-bye the surface
	SDL_FreeSurface(surface);

	// Creating the destiny position for the car image (once the mouse button is clicked those variables will change)
	int carDestinyX = 25;
	int carDestinyY = 25;
	int destinyReachedAcceptanceDist = 2;
	int cellDestinyX = 0;
	int cellDestinyY = 0;
	int cellTempDestinyX = 0;
	int cellTempDestinyY = 0;
	bool canMoveToValidCell = false;

	int done = 0;
	SDL_Event sdl_event;

	float deltaTime = 0.f;
	float lastTick = 0.f;

	// The main loop
	// Every iteration is a frame
	while (!done)
	{
		float currentTick = (float)SDL_GetTicks() / 1000.f;
		deltaTime = currentTick - lastTick;
		lastTick = currentTick;

		// Polling the messages from the OS.
		// That could be key downs, mouse movement, ALT+F4 or many others
		while (SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_QUIT) // The user wants to quit
			{
				done = 1;
			}
			else if (sdl_event.type == SDL_KEYDOWN) // A key was pressed
			{
				switch (sdl_event.key.keysym.sym) // Which key?
				{
				case SDLK_ESCAPE: { // Posting a quit message to the OS queue so it gets processed on the next step and closes the game
					SDL_Event event;
					event.type = SDL_QUIT;
					event.quit.type = SDL_QUIT;
					event.quit.timestamp = SDL_GetTicks();
					SDL_PushEvent(&event);
				}
								break;
								// Other keys here
				default:
					break;
				}
			}
			else if (sdl_event.type == SDL_MOUSEBUTTONDOWN) { // Mouse button was clicked
				switch (sdl_event.button.button) { // Check the type of button clicked
				case SDL_BUTTON_LEFT: { // Left mouse button
					// Get the current position of the mouse and update the destiny position for the car
					int mousePosX = 0;
					int mousePosY = 0;
					SDL_GetMouseState(&mousePosX, &mousePosY);
					cellDestinyX = mousePosX / CELL_SIZE;
					cellDestinyY = mousePosY / CELL_SIZE;

					initCells(&board);

					// Grassfire algorithm
					if (board.cells[cellDestinyY][cellDestinyX] != 255)
					{
						canMoveToValidCell = true;
						board.cells[cellDestinyY][cellDestinyX] = 1;
						bool S = true;
						while (S)
						{
							S = false;
							memcpy(board.cellsOld, board.cells, sizeof(board.cells));
							for (int i = 0; i < CELLS_Y; ++i)
							{
								for (int j = 0; j < CELLS_X; ++j)
								{
									int A = board.cellsOld[i][j];
									if (A != 0 && A != 255)
									{
										int B = A + 1;
										// Adjacent top
										if (i > 0)
										{
											int x = board.cellsOld[i - 1][j];
											if (x == 0)
											{
												board.cells[i - 1][j] = B;
												S = true;
											}
										}
										// Adjacent right
										if (j < CELLS_X - 1)
										{
											int x = board.cellsOld[i][j + 1];
											if (x == 0)
											{
												board.cells[i][j + 1] = B;
												S = true;
											}
										}
										// Adjacent bottom
										if (i < CELLS_Y - 1)
										{
											int x = board.cellsOld[i + 1][j];
											if (x == 0)
											{
												board.cells[i + 1][j] = B;
												S = true;
											}
										}
										// Adjacent left
										if (j > 0)
										{
											int x = board.cellsOld[i][j - 1];
											if (x == 0)
											{
												board.cells[i][j - 1] = B;
												S = true;
											}
										}
									}
								}
							}
						}

						findNextCellDestiny(&car, board.cells, &cellTempDestinyX, &cellTempDestinyY, &carDestinyX, &carDestinyY);
					}
					else
					{
						canMoveToValidCell = false;
					}
				}
									break;
				default: // any other button
					break;
				}
			}
		}

		// Clearing the screen
		SDL_RenderClear(renderer);

		// All drawing goes here

		// Here is the rectangle where the image will be on the screen
		SDL_Rect rect;
		rect.x = (int)round(car.positionX - car.width / 2); // Counting from the image's center but that's up to you
		rect.y = (int)round(car.positionY - car.height / 2); // Counting from the image's center but that's up to you
		rect.w = (int)car.width;
		rect.h = (int)car.height;

		// Draw the board
		for (int i = 0; i < CELLS_Y; ++i)
		{
			for (int j = 0; j < CELLS_X; ++j)
			{
				SDL_Rect cellRect;
				cellRect.x = j * CELL_SIZE;
				cellRect.y = i * CELL_SIZE;
				cellRect.w = CELL_SIZE;
				cellRect.h = CELL_SIZE;

				SDL_Texture* cellTexture = board.cells[i][j] == 255 ? board.obstacleCellTexture : board.defaultCellTexture;
				SDL_RenderCopyEx(renderer, // Already know what is that
					cellTexture, // The image
					0, // A rectangle to crop from the original image. Since we need the whole image that can be left empty (nullptr [0])
					&cellRect, // The destination rectangle on the screen.
					0, // An angle in degrees for rotation
					0, // The center of the rotation (when nullptr [0], the rect center is taken)
					SDL_FLIP_NONE); // We don't want to flip the image
			}
		}

		SDL_RenderCopyEx(renderer, // Already know what is that
			car.texture, // The image
			0, // A rectangle to crop from the original image. Since we need the whole image that can be left empty (nullptr [0])
			&rect, // The destination rectangle on the screen.
			0, // An angle in degrees for rotation
			0, // The center of the rotation (when nullptr [0], the rect center is taken)
			SDL_FLIP_NONE); // We don't want to flip the image

		// Showing the screen to the player
		SDL_RenderPresent(renderer);

		// Prepare the next frame. In our case we need to move the current car position to get closer to the destiny car position
		if (canMoveToValidCell)
		{
			bool reachedTempDestiny = true;
			// Check whether the X current and destiny position differes
			if (abs(car.positionX - carDestinyX) >= destinyReachedAcceptanceDist) {
				// if different then move the car to get closer to its destiny postion
				if (car.positionX > carDestinyX) {
					car.positionX -= car.speed * deltaTime;
				}
				else {
					car.positionX += car.speed * deltaTime;
				}
				reachedTempDestiny = false;
			}
			// Check whether the Y current and destiny position differes
			if (abs(car.positionY - carDestinyY) >= destinyReachedAcceptanceDist) {
				if (car.positionY > carDestinyY) {
					car.positionY -= car.speed * deltaTime;
				}
				else {
					car.positionY += car.speed * deltaTime;
				}
				reachedTempDestiny = false;
			}

			// If reached the current destiny, find the next one until the final destiny is reached
			if (reachedTempDestiny && (cellTempDestinyX != cellDestinyX || cellTempDestinyY != cellDestinyY))
			{
				findNextCellDestiny(&car, board.cells, &cellTempDestinyX, &cellTempDestinyY, &carDestinyX, &carDestinyY);
			}
		}
	}

	// If we reached here then the main loop stoped
	// That means the game wants to quit

	// Shutting down the renderer
	SDL_DestroyRenderer(renderer);

	// Shutting down the window
	SDL_DestroyWindow(window);

	// Quitting the Image SDL library
	IMG_Quit();
	// Quitting the main SDL library
	SDL_Quit();

	// Done.
	return 0;
}

void initCells(Board* board)
{
	// Set all values to 0
	memset(board->cells, 0, sizeof(board->cells));
	// Set the obstacle data
	board->cells[3][5] = 255;
	board->cells[4][5] = 255;
	board->cells[5][5] = 255;
	board->cells[6][5] = 255;
	board->cells[6][6] = 255;
	board->cells[6][7] = 255;

	board->cells[2][11] = 255;
	board->cells[2][12] = 255;
	board->cells[2][13] = 255;
	board->cells[3][13] = 255;
	board->cells[4][13] = 255;
	board->cells[4][12] = 255;
	board->cells[4][11] = 255;
	board->cells[3][11] = 255;
}

void findNextCellDestiny(ObjectData* car, unsigned char cells[CELLS_Y][CELLS_X], int* cellTempDestinyX, int* cellTempDestinyY, int* carDestinyX, int* carDestinyY)
{
	car->currentCellX = *cellTempDestinyX;
	car->currentCellY = *cellTempDestinyY;

	unsigned char minValue = 255;
	// Adjacent left
	if (car->currentCellX > 0)
	{
		unsigned char x = cells[car->currentCellY][car->currentCellX - 1];
		// 0 means that the destiny can't be reached
		if (x < minValue && x != 0)
		{
			minValue = x;
			*cellTempDestinyX = car->currentCellX - 1;
			*cellTempDestinyY = car->currentCellY;
		}
	}
	// Adjacent top
	if (car->currentCellY > 0)
	{
		unsigned char x = cells[car->currentCellY - 1][car->currentCellX];
		if (x < minValue && x != 0)
		{
			minValue = x;
			*cellTempDestinyX = car->currentCellX;
			*cellTempDestinyY = car->currentCellY - 1;
		}
	}
	// Adjacent right
	if (car->currentCellX < CELLS_X - 1)
	{
		unsigned char x = cells[car->currentCellY][car->currentCellX + 1];
		if (x < minValue && x != 0)
		{
			minValue = x;
			*cellTempDestinyX = car->currentCellX + 1;
			*cellTempDestinyY = car->currentCellY;
		}
	}
	// Adjacent bottom
	if (car->currentCellY < CELLS_Y - 1)
	{
		unsigned char x = cells[car->currentCellY + 1][car->currentCellX];
		if (x < minValue && x != 0)
		{
			minValue = x;
			*cellTempDestinyX = car->currentCellX;
			*cellTempDestinyY = car->currentCellY + 1;
		}
	}

	*carDestinyX = (*cellTempDestinyX * CELL_SIZE) + (CELL_SIZE / 2);
	*carDestinyY = (*cellTempDestinyY * CELL_SIZE) + (CELL_SIZE / 2);
}