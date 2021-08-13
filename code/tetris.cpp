/*
TODO:
	- Implement Scores
*/


#include "glad.c"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//GLM - math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "input.h"
#include "tetris_texture.h"

//#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

#define len(arr) (sizeof(arr)) / (sizeof(arr[0]))
#define last(arr) arr[(len(arr))-1]
#define Assert(expression) if(!(expression)) {*(int*)0 = 0;}

#define SET_BIT(a, b) (a) |= (b)
#define RMV_BIT(a, b) (a) &= ~(b);

#define SCR_WIDTH 800
#define SCR_HEIGHT 600
#define CELL_PIXEL_LENGTH 20

#define PLAY_AREA_WIDTH  10
//Actual rendered height is 20, the extra 3 is for blocks that go above when game still running
#define PLAY_AREA_HEIGHT 23 
#define PLAY_AREA_Y_START 3 //Where the play area starts being rendered

#define IVEC2_RIGHT ivec2( 1, 0)
#define IVEC2_LEFT  ivec2(-1, 0)
#define IVEC2_UP	ivec2( 0,-1)
#define IVEC2_DOWN  ivec2( 0, 1)

#define ROTATE_LEFT  true
#define ROTATE_RIGHT false

#define ENABLE_GRID 0

typedef unsigned int uint;
using namespace glm;

enum TetraminoType
{
	NONE,
	SQUARE,
	LINE,
	L_BLOCK,
	RL_BLOCK,
	S_BLOCK,
	RS_BLOCK,
	T_BLOCK
};

struct Tetramino
{
	TetraminoType type;
	ivec2 pos;
	ivec2 blockCoords[4];
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

void DrawCell(uint quadquadVAO, uint shader, int x, int y, vec4 colour);
void DrawBorderedCell(uint quadVAO, uint shader, int x, int y, vec4 fillColour, vec4 borderColour);
void DrawTexturedCell(uint quadVAO, uint texShader, uint texture, int x, int y, vec4 colour);
void ProcessKey(GLFWwindow* window, uint16* key, int glfwKey);

void ResetGame(TetraminoType nextTetraminoType, Tetramino* heldTetramino, TetraminoType tetraminoQueue[5]);
vec4 TetraminoColour(TetraminoType tetraminoType);
void LockTetramino(Tetramino tetramino);
void ClearLines(int lowestRow, int highestRow);
void MoveTetramino(Tetramino* tetramino, ivec2 direction);
void RotateTetramino(Tetramino* tetramino, bool rotateLeft);
ivec2 GetGhostTetraminoPos(Tetramino tetramino);
Tetramino InitTetramino(TetraminoType type, vec2 startPos);
void GetTetraminoBlocks(TetraminoType type, ivec2 blockCoords[4]);
TetraminoType NextTetramino(TetraminoType tetraminoQueue[5]);
ivec2 LocalToCell(ivec2 local);
ivec2 GetBlockCell(Tetramino tetramino, int blockIndex);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Input input;

int playArea[PLAY_AREA_HEIGHT][PLAY_AREA_WIDTH] = {0};

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Initialize window
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tetris", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	//Callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	//Init quad VAO
	float quadVerts[]
    {
		//pos		  //tex
        -1.0f, -1.0f,  0.0f,  0.0f,
        -1.0f,  1.0f,  0.0f,  (float)(SCR_HEIGHT/CELL_PIXEL_LENGTH),
         1.0f, -1.0f,  (float)(SCR_WIDTH/CELL_PIXEL_LENGTH),  0.0f,

         1.0f, -1.0f,  (float)(SCR_WIDTH/CELL_PIXEL_LENGTH),  0.0f,
        -1.0f,  1.0f,  0.0f,  (float)(SCR_HEIGHT/CELL_PIXEL_LENGTH),
         1.0f,  1.0f,  (float)(SCR_WIDTH/CELL_PIXEL_LENGTH),  (float)(SCR_HEIGHT/CELL_PIXEL_LENGTH)    
	};

	uint quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO); 
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

	uint shader = CompileShader("cell", "cell");
	if (shader == SHADER_READ_FILE_ERR)
	{
		std::cout << "FAILED TO READ CELL SHADER FILES!\n";
		return 0;
	}

	uint texShader = CompileShader("texCell", "texCell");
	if (texShader == SHADER_READ_FILE_ERR)
	{
		std::cout << "FAILED TO READ TEX_CELL SHADER FILES!\n";
		return 0;
	}

	uint blockSkin = loadTexture("default_skin.png", true);
	uint fontTexs[26] = {0};
	//for (int i = 0; i < 26; ++i)
	//{
	//	char fileName[6];
	//	char c = (char)(i - 65);
	//	snprintf(fileName, sizeof(fileName), "%c.png", );
	//	fontTexs[i] = loadTexture(fileName, true);
	//}
	fontTexs[6]  = loadTexture("G.png", true);
	fontTexs[0]  = loadTexture("A.png", true);
	fontTexs[12] = loadTexture("M.png", true);
	fontTexs[4]  = loadTexture("E.png", true);
	fontTexs[14] = loadTexture("O.png", true);
	fontTexs[21] = loadTexture("V.png", true);
	fontTexs[17] = loadTexture("R.png", true);


	const int SEEN_HEIGHT = PLAY_AREA_HEIGHT - PLAY_AREA_Y_START;
	const int X_OFFSET = (SCR_WIDTH  - CELL_PIXEL_LENGTH * PLAY_AREA_WIDTH)  / 2;
	const int Y_OFFSET = (SCR_HEIGHT - CELL_PIXEL_LENGTH * SEEN_HEIGHT) / 2 - CELL_PIXEL_LENGTH*3;

	bool playingGame = true;

	float elapsedTime = 0.0f;
	float stepTime = 1.0f;
	float heldTime = 0.0f;
	float moveThreshold = 0.125f;
	int numTimesSamePos = 0;

	std::srand((int)time(nullptr));

	TetraminoType initialType = (TetraminoType)(std::rand() % 7 + 1);
	Tetramino currentTetramino = InitTetramino(initialType, ivec2(4, PLAY_AREA_Y_START+1));
	ivec2 ghostTetraminoPos = GetGhostTetraminoPos(currentTetramino);

	bool changeTetramino = false;
	Tetramino heldTetramino = InitTetramino(NONE, ivec2(0));
	TetraminoType nextTetraminoType;
	TetraminoType tetraminoQueue[5];
	for (int i = 0; i < len(tetraminoQueue); ++i)
		tetraminoQueue[i] =  (TetraminoType)(std::rand() % 7 + 1);

	//for (int i = PLAY_AREA_Y_START+1; i < PLAY_AREA_HEIGHT; ++i)
	//	for (int j = 0; j < PLAY_AREA_WIDTH - 1; ++j)
	//		playArea[i][j] = (int)LINE;		

	//Main Loop
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);	

		if (playingGame)
		{
			//Update Rows	
			if (elapsedTime >= stepTime && !KeyPress(input.down))
			{
				//Auto lock tetramino if been is same position 3 times, else move normally
				if (numTimesSamePos == 2)
				{
					currentTetramino.pos = ghostTetraminoPos;
					LockTetramino(currentTetramino);
					nextTetraminoType = NextTetramino(tetraminoQueue);
					changeTetramino = true;
					numTimesSamePos = 0;
				}
				else
				{
					ivec2 prevPos = currentTetramino.pos;
					MoveTetramino(&currentTetramino, IVEC2_DOWN);
					elapsedTime = 0.0f;

					if (currentTetramino.pos == prevPos)
						numTimesSamePos++;
					else
						numTimesSamePos = 0;
				}
			}

			if (KeyDown(input.left))
			{
				heldTime = 0.0f;
				MoveTetramino(&currentTetramino, IVEC2_LEFT);
			}

			if (KeyDown(input.right))
			{
				heldTime = 0.0f;
				MoveTetramino(&currentTetramino, IVEC2_RIGHT);
			}

			if (KeyPress(input.left))
			{
				heldTime += deltaTime;
				if (heldTime > moveThreshold)
					MoveTetramino(&currentTetramino, IVEC2_LEFT);
			}

			if (KeyPress(input.right))
			{
				heldTime += deltaTime;
				if (heldTime > moveThreshold)
					MoveTetramino(&currentTetramino, IVEC2_RIGHT);
			}

			if (KeyPress(input.down))
			{
				MoveTetramino(&currentTetramino, IVEC2_DOWN);
			}

			if (KeyDown(input.x))
			{
				RotateTetramino(&currentTetramino, ROTATE_RIGHT);
			}

			if (KeyDown(input.z))
			{
				RotateTetramino(&currentTetramino, ROTATE_LEFT);
			}

			ghostTetraminoPos = GetGhostTetraminoPos(currentTetramino);

			if (KeyDown(input.shift))
			{
				TetraminoType newType = (heldTetramino.type != NONE) ? 
											heldTetramino.type : NextTetramino(tetraminoQueue);
				heldTetramino = InitTetramino(currentTetramino.type, ivec2(0));
				nextTetraminoType = newType;
				changeTetramino = true;
				numTimesSamePos = 0;
			}

			if (KeyDown(input.space))
			{
				currentTetramino.pos = ghostTetraminoPos;
				LockTetramino(currentTetramino);
				nextTetraminoType = NextTetramino(tetraminoQueue);
				changeTetramino = true;
				numTimesSamePos = 0;
			}
		}

		if (KeyDown(input.f4))
		{
			ResetGame(nextTetraminoType, &heldTetramino, tetraminoQueue);
			playingGame = true;
			elapsedTime = 0.0f;
			numTimesSamePos = 0;
			changeTetramino = true;
			continue;
		}

		//Draw Black Backgorund
		for (int i = PLAY_AREA_Y_START; i < PLAY_AREA_HEIGHT; ++i)
		{
			for (int j = 0; j < PLAY_AREA_WIDTH; ++j)
			{
				#if ENABLE_GRID
					DrawBorderedCell(quadVAO, shader, X_OFFSET + CELL_PIXEL_LENGTH*j, 
									 SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH*i, 
									 vec4(glm::vec3(0.0f), 1.0f), 
									 vec4(vec3(0.15f), 1.0f));
				#else
					DrawCell(quadVAO, shader, X_OFFSET + CELL_PIXEL_LENGTH*j, 
							 SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH*i, 
							 vec4(glm::vec3(0.0f), 1.0f));
				#endif
			}
		}

		//Draw Play Area
		for (int i = PLAY_AREA_Y_START; i < PLAY_AREA_HEIGHT; ++i)
		{
			for (int j = 0; j < PLAY_AREA_WIDTH; ++j)
			{
				if (playArea[i][j] != 0)
				{
					DrawTexturedCell(quadVAO, texShader, blockSkin, X_OFFSET + CELL_PIXEL_LENGTH*j, 
									 SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH*i, 
								 	 TetraminoColour((TetraminoType)playArea[i][j]));
				}
			}
		}

		//Draw tetramino
		if (!changeTetramino) //Flag here to prevent flashing of tetramino before clear
		{
			for (int i = 0; i < 4; ++i)
			{
				ivec2 cell = GetBlockCell(currentTetramino, i);
				int x = X_OFFSET + CELL_PIXEL_LENGTH * cell.x;
				int y = SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH * cell.y;
				if (cell.y < PLAY_AREA_Y_START) continue;
				DrawTexturedCell(quadVAO, texShader, blockSkin, x, y, TetraminoColour(currentTetramino.type));
			}
		}

		//Draw Ghost Tetramino
		if (!changeTetramino) //Flag here to prevent flashing of tetramino before clear
		{
			for (int i = 0; i < 4; ++i)
			{
				ivec2 cell = ghostTetraminoPos + LocalToCell(currentTetramino.blockCoords[i]);
				int x = X_OFFSET + CELL_PIXEL_LENGTH * cell.x;
				int y = SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH * cell.y;
				if (cell.y < PLAY_AREA_Y_START) continue;
				vec3 ghostColour = TetraminoColour(currentTetramino.type);
				DrawTexturedCell(quadVAO, texShader, blockSkin, x, y, vec4(ghostColour, 0.5f));
			}
		}

		//Draw Held Tetramino
		for (int i = 0; i < 4; ++i)
		{
			if (heldTetramino.type != NONE)
			{
				int x = X_OFFSET - CELL_PIXEL_LENGTH * (3 - heldTetramino.blockCoords[i].x);
				int y = SCR_HEIGHT - Y_OFFSET + CELL_PIXEL_LENGTH * 
							(heldTetramino.blockCoords[i].y - 1 - PLAY_AREA_Y_START);
				DrawTexturedCell(quadVAO, texShader, blockSkin, x, y, TetraminoColour(heldTetramino.type));
			}
		}

		//Draw next tetraminos
		for (int i = 0; i < len(tetraminoQueue); ++i)
		{
			ivec2 blocks[4];
			GetTetraminoBlocks(tetraminoQueue[i], blocks);
			int queueOffset = len(tetraminoQueue) - i - 1;
			for (int b = 0; b < 4; ++b)
			{
				ivec2 cell = LocalToCell(blocks[b]);
				int x = X_OFFSET + CELL_PIXEL_LENGTH * (PLAY_AREA_WIDTH + 3 + cell.x);
				int y = SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH * 
								(3*queueOffset + cell.y + PLAY_AREA_Y_START);
				DrawTexturedCell(quadVAO, texShader, blockSkin, x, y, TetraminoColour(tetraminoQueue[i]));
			}
		}

		//If block goes over height, end the game
		for (int y = 1; y < 3; y++)
		{
			for (int i = 0; i < len(playArea[PLAY_AREA_Y_START-y]); ++i)
			{
				if (playArea[PLAY_AREA_Y_START-1][i] != 0)
				{
					playingGame = false;
					break;
				}
			}
		}

		if (!playingGame)
		{
			const char gameOver[10] = "GAME OVER";
			for (int i = 0; i < len(gameOver); ++i)
			{
				int charIndex = (int)gameOver[i] - 65;
				int x = X_OFFSET + CELL_PIXEL_LENGTH * i;
				int y = SCR_HEIGHT - Y_OFFSET + CELL_PIXEL_LENGTH; 

				if (charIndex >= 0 && charIndex < 26)
					DrawTexturedCell(quadVAO, texShader, fontTexs[charIndex], x, y, vec4(1.0f));
				
			}
		}


		if (changeTetramino && playingGame)
		{
			currentTetramino = InitTetramino(nextTetraminoType, ivec2(4, PLAY_AREA_Y_START+1));
			
			//Offset tetramino if usual spawn pos is unavailable
			int offset = 0;
			int start = min(currentTetramino.blockCoords[0].x, currentTetramino.blockCoords[3].x);
			int end = max(currentTetramino.blockCoords[0].x, currentTetramino.blockCoords[3].x);
			for (int i = PLAY_AREA_Y_START+2; i >= PLAY_AREA_Y_START; --i)
			{
				for (int j = start; j < end; ++j)
				{
					int x = 4 + j;
					if (playArea[i][x] > 0)
					{
						offset++;
						break;
					}
				}
			}
			currentTetramino.pos.y -= offset;
			
			changeTetramino = false;
		}


		elapsedTime += deltaTime;
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}



//Called when window size changed
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	ProcessKey(window, &input.z, GLFW_KEY_Z);
	ProcessKey(window, &input.x, GLFW_KEY_X);
	
	ProcessKey(window, &input.down, GLFW_KEY_DOWN);
	ProcessKey(window, &input.left, GLFW_KEY_LEFT);
	ProcessKey(window, &input.right, GLFW_KEY_RIGHT);

	ProcessKey(window, &input.space, GLFW_KEY_SPACE);
	ProcessKey(window, &input.shift, GLFW_KEY_LEFT_SHIFT);
	ProcessKey(window, &input.escape, GLFW_KEY_ESCAPE);
	ProcessKey(window, &input.f4, GLFW_KEY_F4);
}

void DrawCell(uint quadVAO, uint shader, int x, int y, vec4 colour)
{
    glUseProgram(shader);
	shader_SetVector4f(shader, "spriteColour", colour);

    glBindVertexArray(quadVAO);
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, CELL_PIXEL_LENGTH, CELL_PIXEL_LENGTH);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_SCISSOR_TEST);
}

void DrawBorderedCell(uint quadVAO, uint shader, int x, int y, vec4 fillColour, vec4 borderColour)
{
	glEnable(GL_SCISSOR_TEST);
	glUseProgram(shader);
	
	//Draw border
	shader_SetVector4f(shader, "spriteColour", borderColour);
    glBindVertexArray(quadVAO);
	glScissor(x, y, CELL_PIXEL_LENGTH, CELL_PIXEL_LENGTH);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//Draw centre
	shader_SetVector4f(shader, "spriteColour", fillColour);
    glScissor(x + 1, y + 1, CELL_PIXEL_LENGTH - 2, CELL_PIXEL_LENGTH - 2);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_SCISSOR_TEST);
}

void DrawTexturedCell(uint quadVAO, uint texShader, uint texture, int x, int y, vec4 colour)
{
	glUseProgram(texShader);
	shader_SetVector4f(texShader, "spriteColour", colour);
	shader_SetInt(texShader, "tex", 0);

	glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(quadVAO);
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, CELL_PIXEL_LENGTH, CELL_PIXEL_LENGTH);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_SCISSOR_TEST);
}

void ProcessKey(GLFWwindow* window, uint16* key,int glfwKey)
{
	bool wasPressed = KeyPress(*key);
	bool isPressed  = glfwGetKey(window, glfwKey) == GLFW_PRESS;

	if ( isPressed) SET_BIT(*key, KEY_PRESS);
	if (!isPressed) RMV_BIT(*key, KEY_PRESS);

	if (!wasPressed &&  isPressed) SET_BIT(*key, KEY_DOWN);
	if ( wasPressed && !isPressed) SET_BIT(*key, KEY_UP);
	if ( wasPressed &&  isPressed) RMV_BIT(*key, KEY_DOWN);
	if (!wasPressed && !isPressed) *key = KEY_NULL;
}


void ResetGame(TetraminoType nextTetraminoType, 
			   Tetramino* heldTetramino, 
			   TetraminoType tetraminoQueue[5])
{
	nextTetraminoType = (TetraminoType)(std::rand() % 7 + 1);
	*heldTetramino = InitTetramino(NONE, ivec2(0));
	
	for (int i = 0; i < 5; ++i)
		tetraminoQueue[i] =  (TetraminoType)(std::rand() % 7 + 1);
	
	for (int i = 0; i < PLAY_AREA_HEIGHT; ++i)
		for (int j = 0; j < PLAY_AREA_WIDTH; ++j)
			playArea[i][j] = 0;
}

vec4 TetraminoColour(TetraminoType tetraminoType)
{
	switch (tetraminoType)
	{
		default:       return vec4(vec3(0.0f), 1.0f);
		case SQUARE:   return vec4(1.0f, 1.0f, 0.0f, 1.0f);
		case LINE:     return vec4(0.0f, 1.0f, 1.0f, 1.0f);
		case L_BLOCK:  return vec4(1.0f, 153.0f/255.0f, 0.0f, 1.0f);
		case RL_BLOCK: return vec4(0.0f, 0.0f, 1.0f, 1.0f);
		case S_BLOCK:  return vec4(0.0f, 1.0f, 0.0f, 1.0f);
		case RS_BLOCK: return vec4(1.0f, 0.0f, 0.0f, 1.0f);
		case T_BLOCK:  return vec4(153.0f/255.0f, 51.0f/255.0f, 1.0f, 1.0f);
	}
}

//Places tetramino blocks into play area
void LockTetramino(Tetramino tetramino)
{
	for (int i = 0; i < 4; i++)
	{
		ivec2 cell = GetBlockCell(tetramino, i);
		Assert(cell.y >= 0);
		playArea[cell.y][cell.x] = (int)tetramino.type;
	}

	//Get highest and lowest y index of tetramino
	int highestY = 0;
	int lowestY = PLAY_AREA_HEIGHT + 1;
	for (int i = 0; i < 4; i++)
	{
		int y = GetBlockCell(tetramino, i).y;
		if (y > highestY) highestY = y;
		if (y < lowestY)  lowestY  = y;
	}
	ClearLines(lowestY, highestY);
}

void ClearLines(int lowestRow, int highestRow)
{
	bool lineHasBeenCleared = false;
	for (int r = lowestRow; r <= highestRow; ++r)
	{
		bool clear = true;
		for (int c = 0; c < PLAY_AREA_WIDTH; ++c)
		{
			if (playArea[r][c] == 0)
			{
				clear = false;
				break;
			}
		}

		if (clear)
		{
			for (int j = r; j > 0; --j)
				for (int c = 0; c < PLAY_AREA_WIDTH; ++c) 
					playArea[j][c] = playArea[j-1][c];
		} 
	} 
}

void MoveTetramino(Tetramino* tetramino, ivec2 direction)
{	
	for (int i = 0; i < 4; ++i)
	{
		ivec2 newCell = GetBlockCell(*tetramino, i) + direction;

		//Check that block within play area
		if (newCell.x >= PLAY_AREA_WIDTH || newCell.x < 0 || newCell.y >= PLAY_AREA_HEIGHT) return;
		//Check that block does not intersect piece
		if (playArea[newCell.y][newCell.x] > 0) return;
	}
	tetramino->pos += direction;
}

//Wall kicks are a bit dodgy, fix possibly
void RotateTetramino(Tetramino* tetramino, bool rotateLeft)
{
	if (tetramino->type == SQUARE) return;

	//Get Rotation
	ivec2 rotatedBlocks[4];
	for (int i = 0; i < 4; ++i)
	{
		ivec2 rot; 
		rot.x = tetramino->blockCoords[i].y;
		rot.y = tetramino->blockCoords[i].x;
		if (tetramino->type != LINE)
		{
			if (rotateLeft) rot.x *= -1;
			else 			rot.y *= -1;	
		}
		rotatedBlocks[i] = rot;
	}

	//Push piece back into bounds if out of bounds
	ivec2 offset = ivec2(0);
	for (int i = 0; i < 4; ++i)
	{
		ivec2 cell = tetramino->pos + LocalToCell(rotatedBlocks[i]);

		int rDiff = cell.x - (PLAY_AREA_WIDTH - 1);
		int lDiff = cell.x - 0;
		int yDiff = cell.y - (PLAY_AREA_HEIGHT - 1);
		if (rDiff > offset.x) offset.x = rDiff;
		if (lDiff < offset.x) offset.x = lDiff;
		if (yDiff > offset.y) offset.y = yDiff;
	}
	tetramino->pos -= offset;

	//If rotation would intersect other block, apply wall kick
	ivec2 kickDirections[5] = {ivec2(0), IVEC2_LEFT, IVEC2_RIGHT, IVEC2_UP, IVEC2_DOWN};
	int k = 0;
	while (k < 5)
	{
		int numFit = 0;
		for (int i = 0; i < 4; ++i)
		{
			ivec2 newCell = tetramino->pos + LocalToCell(rotatedBlocks[i]) + kickDirections[k];
			
			bool inBounds = newCell.x < PLAY_AREA_WIDTH && newCell.x >= 0;
			numFit += (playArea[newCell.y][newCell.x] == 0 && inBounds); 
		}

		if (numFit == 4) break;  //Piece can fit
		else if (k == 4) return; //Piece cannot fit
		++k;
	}
	for (int i = 0; i < 4; ++i) tetramino->blockCoords[i] = rotatedBlocks[i];
	tetramino->pos += kickDirections[k];

	
}

ivec2 GetGhostTetraminoPos(Tetramino tetramino)
{
	//Find lowest hanging block
	int maxY = 0;
	for (int i = 0; i < 4; ++i)
	{
		if (GetBlockCell(tetramino, i).y > maxY)
			maxY = GetBlockCell(tetramino, i).y;
	}

	//Cast rays below tetramino blocks until collision is detected
	int y = 0;
	while (y < PLAY_AREA_HEIGHT - maxY)
	{
		bool intersecting = false;
		for (int i = 0; i < 4; ++i)
		{
			ivec2 cell = GetBlockCell(tetramino, i);
			if (playArea[cell.y + y][cell.x] > 0) 
			{
				intersecting = true;
				break;
			}
		}

		if (intersecting) break;
		++y;
	}

	//Return position + (length of ray - 1) 
	return tetramino.pos + IVEC2_DOWN * (y - 1);
}

Tetramino InitTetramino(TetraminoType type, vec2 pos)
{
	Tetramino tetramino;
	tetramino.type = type;
	tetramino.pos = pos;
	GetTetraminoBlocks(type, tetramino.blockCoords);
	return tetramino;
}

void GetTetraminoBlocks(TetraminoType type, ivec2 blockCoords[4])
{
	//All the "blockCoords" are relative to their centre except the square and line are 
	//relative to their centre. Square is relative to top left and line is relative to
	//third block
	switch (type)
	{
		default:
			for (int i = 0; i < 4; ++i) 
				blockCoords[i] = ivec2(0);
			break;
		case SQUARE:   
			blockCoords[0] = ivec2( 0,  0);
			blockCoords[1] = ivec2( 1,  0);
			blockCoords[2] = ivec2( 0,  1);
			blockCoords[3] = ivec2( 1,  1);
			break;
		case LINE:
			blockCoords[0] = ivec2(-2,  0);
			blockCoords[1] = ivec2(-1,  0);
			blockCoords[2] = ivec2( 0,  0);
			blockCoords[3] = ivec2( 1,  0);
			break;
		case L_BLOCK:  
			blockCoords[0] = ivec2( 1,  1);
			blockCoords[1] = ivec2( 1,  0);
			blockCoords[2] = ivec2( 0,  0);
			blockCoords[3] = ivec2(-1,  0);
			break;
		case RL_BLOCK: 
			blockCoords[0] = ivec2(-1,  1);
			blockCoords[1] = ivec2(-1,  0);
			blockCoords[2] = ivec2( 0,  0);
			blockCoords[3] = ivec2( 1,  0);
			break;
		case S_BLOCK: 
			blockCoords[0] = ivec2(-1,  0);
			blockCoords[1] = ivec2( 0,  0);
			blockCoords[2] = ivec2( 0,  1);
			blockCoords[3] = ivec2( 1,  1);
			break;
		case RS_BLOCK: 
			blockCoords[0] = ivec2(-1,  1);
			blockCoords[1] = ivec2( 0,  1);
			blockCoords[2] = ivec2( 0,  0);
			blockCoords[3] = ivec2( 1,  0);
			break;
		case T_BLOCK:  
			blockCoords[0] = ivec2(-1,  0);
			blockCoords[1] = ivec2( 0,  0);
			blockCoords[2] = ivec2( 0,  1);
			blockCoords[3] = ivec2( 1,  0);
			break;
	}
}

//Think of making queue?
TetraminoType NextTetramino(TetraminoType tetraminoQueue[5])
{
	TetraminoType result = tetraminoQueue[4];

	//set curr type to their prev one
	for (int i = 4; i > 0; --i)
		tetraminoQueue[i] = tetraminoQueue[i-1];

	tetraminoQueue[0] = (TetraminoType)(std::rand() % 7 + 1);
	
	return result;
}

ivec2 LocalToCell(ivec2 local)
{
	return ivec2(local.x, -local.y);
}

ivec2 GetBlockCell(Tetramino tetramino, int blockIndex)
{
	return tetramino.pos + LocalToCell(tetramino.blockCoords[blockIndex]);
}
