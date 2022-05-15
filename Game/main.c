#include <stdio.h>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_image.h"
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

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
	int destinyX;
	int destinyY;
	int destinyReachedAcceptanceDist;
	int cellDestinyX;
	int cellDestinyY;
	int cellTempDestinyX;
	int cellTempDestinyY;
	bool canMoveToValidCell;
	bool moving;
};
typedef struct ObjectData ObjectData; // Creating shortcut for the struct that is declared above

struct Coords
{
	int x;
	int y;
};
typedef struct Coords Coords;

#define CELLS_X 15
#define CELLS_Y 11
#define OBSTACLES_COUNT 10

struct Board
{
	SDL_Texture* defaultCellTexture;
	SDL_Texture* obstacleCellTexture;
	unsigned char cells[CELLS_Y][CELLS_X];
	unsigned char cellsOld[CELLS_Y][CELLS_X];
	unsigned char cellsWithoutCharacters[CELLS_Y][CELLS_X];
	Coords obstacles[OBSTACLES_COUNT];
};
typedef struct Board Board;

#define CELL_SIZE 50
#define CHARACTERS_COUNT 8

// Global variables that contains window size 
int WINDOW_WIDTH = 15 * CELL_SIZE;
int WINDOW_HEIGHT = 11 * CELL_SIZE;

void initCharacters(ObjectData* characters, SDL_Texture* texture, int startCellX, int startCellY);
void generateObstacles(Board* board);
bool doesObstacleAlreadyExist(Board* board, int x, int y, int i);
void initCells(Board* board, ObjectData* playerCharacters, ObjectData* enemyCharacters);
void generateRandomDestination(ObjectData* character, Board* board);
void findNextCellDestiny(ObjectData* character, unsigned char cells[CELLS_Y][CELLS_X]);
void grassfireAlgorithm(Board* board, ObjectData* character);
void drawBoard(Board* board, SDL_Renderer* renderer);
void drawCharacters(ObjectData* characters, SDL_Renderer* renderer);
void drawCurrentCharacterHighlight(ObjectData* currentCharacter, SDL_Texture* texture, SDL_Renderer* renderer);

int main()
{
	srand(time(NULL));

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

	// Here the surface is the information about the image. It contains the color data, width, height and other info.
	SDL_Surface* surface = IMG_Load("playerCharacter.png");
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", "image.png", IMG_GetError());
		return -1;
	}

	// Now we use the renderer and the surface to create a texture which we later can draw on the screen.
	SDL_Texture* playerTexture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!playerTexture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return -1;
	}

	// Bye-bye the surface
	SDL_FreeSurface(surface);

	ObjectData playerCharacters[CHARACTERS_COUNT];
	initCharacters(playerCharacters, playerTexture, 0, 0);

	surface = IMG_Load("enemyCharacter.png");
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", "image.png", IMG_GetError());
		return -1;
	}

	// Now we use the renderer and the surface to create a texture which we later can draw on the screen.
	SDL_Texture* enemyTexture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!enemyTexture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return -1;
	}

	ObjectData enemyCharacters[CHARACTERS_COUNT];
	initCharacters(enemyCharacters, enemyTexture, CELLS_X - 1, 0);

	Board board;
	generateObstacles(&board);
	initCells(&board, playerCharacters, enemyCharacters);
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

	surface = IMG_Load("highlight.png");
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", "cellObstacle.png", IMG_GetError());
		return -1;
	}

	// Now we use the renderer and the surface to create a texture which we later can draw on the screen.
	SDL_Texture* highlightTexture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!highlightTexture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return -1;
	}
	// Bye-bye the surface
	SDL_FreeSurface(surface);

	bool playerTurn = true;
	int currentCharacterIdx = 0;
	ObjectData* currentCharacter = &playerCharacters[0];

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
					if (playerTurn)
					{
						int mousePosX = 0;
						int mousePosY = 0;
						SDL_GetMouseState(&mousePosX, &mousePosY);
						currentCharacter->cellDestinyX = mousePosX / CELL_SIZE;
						currentCharacter->cellDestinyY = mousePosY / CELL_SIZE;

						initCells(&board, playerCharacters, enemyCharacters);

						grassfireAlgorithm(&board, currentCharacter);
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

		drawBoard(&board, renderer);
		drawCharacters(playerCharacters, renderer);
		drawCharacters(enemyCharacters, renderer);
		drawCurrentCharacterHighlight(currentCharacter, highlightTexture, renderer);

		// Showing the screen to the player
		SDL_RenderPresent(renderer);

		// Prepare the next frame. In our case we need to move the current car position to get closer to the destiny car position
		if (currentCharacter->canMoveToValidCell)
		{
			bool reachedTempDestiny = true;
			// Check whether the X current and destiny position differes
			if (abs(currentCharacter->positionX - currentCharacter->destinyX) >= currentCharacter->destinyReachedAcceptanceDist) {
				// if different then move the car to get closer to its destiny postion
				if (currentCharacter->positionX > currentCharacter->destinyX) {
					currentCharacter->positionX -= currentCharacter->speed * deltaTime;
				}
				else {
					currentCharacter->positionX += currentCharacter->speed * deltaTime;
				}
				reachedTempDestiny = false;
			}
			// Check whether the Y current and destiny position differes
			if (abs(currentCharacter->positionY - currentCharacter->destinyY) >= currentCharacter->destinyReachedAcceptanceDist) {
				if (currentCharacter->positionY > currentCharacter->destinyY) {
					currentCharacter->positionY -= currentCharacter->speed * deltaTime;
				}
				else {
					currentCharacter->positionY += currentCharacter->speed * deltaTime;
				}
				reachedTempDestiny = false;
			}

			// If reached the current destiny, find the next one until the final destiny is reached
			if (reachedTempDestiny &&
				(currentCharacter->cellTempDestinyX != currentCharacter->cellDestinyX || currentCharacter->cellTempDestinyY != currentCharacter->cellDestinyY))
			{
				findNextCellDestiny(currentCharacter, board.cells);
			}
			else if (reachedTempDestiny
				&& currentCharacter->cellTempDestinyX == currentCharacter->cellDestinyX
				&& currentCharacter->cellTempDestinyY == currentCharacter->cellDestinyY
				&& currentCharacter->moving)
			{
				currentCharacter->moving = false;
				currentCharacter->currentCellX = currentCharacter->cellDestinyX;
				currentCharacter->currentCellY = currentCharacter->cellDestinyY;

				if (playerTurn)
				{
					playerTurn = false;
					currentCharacter = &enemyCharacters[currentCharacterIdx];
					initCells(&board, playerCharacters, enemyCharacters);
					generateRandomDestination(currentCharacter, &board);
				}
				else
				{
					playerTurn = true;
					++currentCharacterIdx;
					if (currentCharacterIdx == CHARACTERS_COUNT)
					{
						currentCharacterIdx = 0;
					}
					currentCharacter = &playerCharacters[currentCharacterIdx];
				}
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

void initCharacters(ObjectData* characters, SDL_Texture* texture, int startCellX, int startCellY)
{
	float posX = startCellX * CELL_SIZE + (CELL_SIZE / 2);
	float posY = startCellY * CELL_SIZE + (CELL_SIZE / 2);

	for (int i = 0; i < CHARACTERS_COUNT; ++i)
	{
		characters[i].texture = texture;
		characters[i].positionX = posX;
		characters[i].positionY = posY;
		characters[i].width = CELL_SIZE;
		characters[i].height = CELL_SIZE;
		characters[i].speed = 200.f;
		characters[i].currentCellX = startCellX;
		characters[i].currentCellY = startCellY;
		characters[i].destinyX = (int)posX;
		characters[i].destinyY = (int)posY;
		characters[i].destinyReachedAcceptanceDist = 2;
		characters[i].cellDestinyX = startCellX;
		characters[i].cellDestinyY = startCellY;
		characters[i].cellTempDestinyX = startCellX;
		characters[i].cellTempDestinyY = startCellY;
		characters[i].canMoveToValidCell = false;
		characters[i].moving = false;

		++startCellY;
		posY += CELL_SIZE;
	}
}

void generateObstacles(Board* board)
{
	Coords maxCoords;
	maxCoords.x = 10;
	maxCoords.y = 7;
	Coords minCoords;
	minCoords.x = 4;
	minCoords.y = 3;
	const int totalX = maxCoords.x - minCoords.x + 1;
	const int totalY = maxCoords.y - minCoords.y + 1;

	for (int i = 0; i < OBSTACLES_COUNT; ++i)
	{
		int x = rand() % totalX + minCoords.x;
		int y = rand() % totalY + minCoords.y;
		while (doesObstacleAlreadyExist(board, x, y, i))
		{
			x = rand() % totalX + minCoords.x;
			y = rand() % totalY + minCoords.y;
		}
		board->obstacles[i].x = x;
		board->obstacles[i].y = y;
	}
}

bool doesObstacleAlreadyExist(Board* board, int x, int y, int i)
{
	for (int j = 0; j < i; ++j)
	{
		if (x == board->obstacles[j].x && y == board->obstacles[j].y)
		{
			return true;
		}
	}
	return false;
}

void initCells(Board* board, ObjectData* playerCharacters, ObjectData* enemyCharacters)
{
	// Set all values to 0
	memset(board->cells, 0, sizeof(board->cells));
	// Set the obstacle data
	for (int i = 0; i < OBSTACLES_COUNT; ++i)
	{
		board->cells[board->obstacles[i].y][board->obstacles[i].x] = 255;
	}

	memcpy(board->cellsWithoutCharacters, board->cells, sizeof(board->cells));

	// Characters as obstacles
	for (int i = 0; i < CHARACTERS_COUNT; ++i)
	{
		board->cells[playerCharacters[i].currentCellY][playerCharacters[i].currentCellX] = 255;
	}
	for (int i = 0; i < CHARACTERS_COUNT; ++i)
	{
		board->cells[enemyCharacters[i].currentCellY][enemyCharacters[i].currentCellX] = 255;
	}
}

void findNextCellDestiny(ObjectData* character, unsigned char cells[CELLS_Y][CELLS_X])
{
	character->currentCellX = character->cellTempDestinyX;
	character->currentCellY = character->cellTempDestinyY;

	unsigned char minValue = 255;
	// Adjacent left
	if (character->currentCellX > 0)
	{
		unsigned char x = cells[character->currentCellY][character->currentCellX - 1];
		// 0 means that the destiny can't be reached
		if (x < minValue && x != 0)
		{
			minValue = x;
			character->cellTempDestinyX = character->currentCellX - 1;
			character->cellTempDestinyY = character->currentCellY;
		}
	}
	// Adjacent top
	if (character->currentCellY > 0)
	{
		unsigned char x = cells[character->currentCellY - 1][character->currentCellX];
		if (x < minValue && x != 0)
		{
			minValue = x;
			character->cellTempDestinyX = character->currentCellX;
			character->cellTempDestinyY = character->currentCellY - 1;
		}
	}
	// Adjacent right
	if (character->currentCellX < CELLS_X - 1)
	{
		unsigned char x = cells[character->currentCellY][character->currentCellX + 1];
		if (x < minValue && x != 0)
		{
			minValue = x;
			character->cellTempDestinyX = character->currentCellX + 1;
			character->cellTempDestinyY = character->currentCellY;
		}
	}
	// Adjacent bottom
	if (character->currentCellY < CELLS_Y - 1)
	{
		unsigned char x = cells[character->currentCellY + 1][character->currentCellX];
		if (x < minValue && x != 0)
		{
			minValue = x;
			character->cellTempDestinyX = character->currentCellX;
			character->cellTempDestinyY = character->currentCellY + 1;
		}
	}

	character->destinyX = (character->cellTempDestinyX * CELL_SIZE) + (CELL_SIZE / 2);
	character->destinyY = (character->cellTempDestinyY * CELL_SIZE) + (CELL_SIZE / 2);
}

void grassfireAlgorithm(Board* board, ObjectData* character)
{
	// Grassfire algorithm
	if (board->cells[character->cellDestinyY][character->cellDestinyX] != 255)
	{
		character->canMoveToValidCell = true;
		character->moving = true;
		board->cells[character->cellDestinyY][character->cellDestinyX] = 1;
		bool S = true;
		while (S)
		{
			S = false;
			memcpy(board->cellsOld, board->cells, sizeof(board->cells));
			for (int i = 0; i < CELLS_Y; ++i)
			{
				for (int j = 0; j < CELLS_X; ++j)
				{
					int A = board->cellsOld[i][j];
					if (A != 0 && A != 255)
					{
						int B = A + 1;
						// Adjacent top
						if (i > 0)
						{
							int x = board->cellsOld[i - 1][j];
							if (x == 0)
							{
								board->cells[i - 1][j] = B;
								S = true;
							}
						}
						// Adjacent right
						if (j < CELLS_X - 1)
						{
							int x = board->cellsOld[i][j + 1];
							if (x == 0)
							{
								board->cells[i][j + 1] = B;
								S = true;
							}
						}
						// Adjacent bottom
						if (i < CELLS_Y - 1)
						{
							int x = board->cellsOld[i + 1][j];
							if (x == 0)
							{
								board->cells[i + 1][j] = B;
								S = true;
							}
						}
						// Adjacent left
						if (j > 0)
						{
							int x = board->cellsOld[i][j - 1];
							if (x == 0)
							{
								board->cells[i][j - 1] = B;
								S = true;
							}
						}
					}
				}
			}
		}

		findNextCellDestiny(character, board->cells);
	}
	else
	{
		character->canMoveToValidCell = false;
	}
}

void generateRandomDestination(ObjectData* character, Board* board)
{
	int x = rand() % CELLS_X;
	int y = rand() % CELLS_Y;
	while (board->cells[y][x] == 255)
	{
		int x = rand() % CELLS_X;
		int y = rand() % CELLS_Y;
	}
	character->cellDestinyX = x;
	character->cellDestinyY = y;
	grassfireAlgorithm(board, character);
}

void drawBoard(Board* board, SDL_Renderer* renderer)
{
	for (int i = 0; i < CELLS_Y; ++i)
	{
		for (int j = 0; j < CELLS_X; ++j)
		{
			SDL_Rect cellRect;
			cellRect.x = j * CELL_SIZE;
			cellRect.y = i * CELL_SIZE;
			cellRect.w = CELL_SIZE;
			cellRect.h = CELL_SIZE;

			SDL_Texture* cellTexture = board->cellsWithoutCharacters[i][j] == 255 ? board->obstacleCellTexture : board->defaultCellTexture;
			SDL_RenderCopyEx(renderer, // Already know what is that
				cellTexture, // The image
				0, // A rectangle to crop from the original image. Since we need the whole image that can be left empty (nullptr [0])
				&cellRect, // The destination rectangle on the screen.
				0, // An angle in degrees for rotation
				0, // The center of the rotation (when nullptr [0], the rect center is taken)
				SDL_FLIP_NONE); // We don't want to flip the image
		}
	}
}

void drawCharacters(ObjectData* characters, SDL_Renderer* renderer)
{
	for (int i = 0; i < CHARACTERS_COUNT; ++i)
	{
		// Here is the rectangle where the image will be on the screen
		SDL_Rect rect;
		rect.x = (int)round(characters[i].positionX - characters[i].width / 2); // Counting from the image's center but that's up to you
		rect.y = (int)round(characters[i].positionY - characters[i].height / 2); // Counting from the image's center but that's up to you
		rect.w = characters[i].width;
		rect.h = characters[i].height;

		SDL_RenderCopyEx(renderer, // Already know what is that
			characters[i].texture, // The image
			0, // A rectangle to crop from the original image. Since we need the whole image that can be left empty (nullptr [0])
			&rect, // The destination rectangle on the screen.
			0, // An angle in degrees for rotation
			0, // The center of the rotation (when nullptr [0], the rect center is taken)
			SDL_FLIP_NONE); // We don't want to flip the image
	}
}

void drawCurrentCharacterHighlight(ObjectData* currentCharacter, SDL_Texture* texture, SDL_Renderer* renderer)
{
	SDL_Rect rect;
	rect.x = (int)round(currentCharacter->positionX - currentCharacter->width / 2); // Counting from the image's center but that's up to you
	rect.y = (int)round(currentCharacter->positionY - currentCharacter->height / 2); // Counting from the image's center but that's up to you
	rect.w = currentCharacter->width;
	rect.h = currentCharacter->height;

	SDL_RenderCopyEx(renderer, // Already know what is that
		texture, // The image
		0, // A rectangle to crop from the original image. Since we need the whole image that can be left empty (nullptr [0])
		&rect, // The destination rectangle on the screen.
		0, // An angle in degrees for rotation
		0, // The center of the rotation (when nullptr [0], the rect center is taken)
		SDL_FLIP_NONE); // We don't want to flip the image
}