#include "glad.c"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "tetris_vecs.h"
#include "shader.h"
#include "input.h"
#include "tetris_texture.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

#define len(arr) (sizeof(arr)) / (sizeof(arr[0]))
#define last(arr) arr[(len(arr))-1]
#if DEBUG
#define Assert(expression) if(!(expression)) {*(int*)0 = 0;}
#else
#define Assert(expression)
#endif

#define SET_BIT(a, b) (a) |= (b)
#define RMV_BIT(a, b) (a) &= ~(b);

#define SCR_WIDTH 800
#define SCR_HEIGHT 600
#define CELL_PIXEL_LENGTH 20

#define PLAY_AREA_WIDTH  10
//Actual rendered height is 20, the extra 3 is for blocks that go above when game still running
#define PLAY_AREA_HEIGHT 23 
#define PLAY_AREA_Y_START 3 //Where the play area starts being rendered
#define X_OFFSET (SCR_WIDTH  - CELL_PIXEL_LENGTH * PLAY_AREA_WIDTH) / 2
#define Y_OFFSET (SCR_HEIGHT / CELL_PIXEL_LENGTH - PLAY_AREA_HEIGHT - PLAY_AREA_Y_START) * CELL_PIXEL_LENGTH / 2 

#define VEC2I_RIGHT vec2i_init( 1, 0)
#define VEC2I_LEFT  vec2i_init(-1, 0)
#define VEC2I_UP	vec2i_init( 0,-1)
#define VEC2I_DOWN  vec2i_init( 0, 1)

#define ROTATE_LEFT  true
#define ROTATE_RIGHT false

#define NUM_TEX_OFFSET 22

typedef unsigned int uint;
//using namespace glm;

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
	vec2i pos;
	vec2i blockCoords[4];
};

struct DrawBuffer
{
	uint vao;
	uint shader;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

void DrawCell(DrawBuffer draw, vec2i pixelPos, vec4 colour);
void DrawBorderedCell(DrawBuffer buffer, vec2i pixelPos, vec4 fillColour, vec4 borderColour);
void DrawTexturedCell(DrawBuffer buffer, uint texture, vec2i pixelPos, vec4 colour);
void DrawText(DrawBuffer buffer, uint fontTexs[36], 
			  const char* text, int textLength, vec2i pixelPos, vec4 colour);
vec2i WorldToPixel(vec2i localPos, bool applyXOffset = true);
void ProcessKey(GLFWwindow* window, uint16* key, int glfwKey);

void ResetGame(TetraminoType nextTetraminoType, Tetramino* heldTetramino, 
			   TetraminoType tetraminoQueue[5], int* score);
vec4 TetraminoColour(TetraminoType tetraminoType);
void LockTetramino(Tetramino tetramino, int* score);
void ClearLines(int lowestRow, int highestRow, int* score);
void MoveTetramino(Tetramino* tetramino, vec2i direction);
void RotateTetramino(Tetramino* tetramino, bool rotateLeft);
vec2i GetGhostTetraminoPos(Tetramino tetramino);
Tetramino InitTetramino(TetraminoType type, vec2i startPos);
void GetTetraminoBlocks(TetraminoType type, vec2i blockCoords[4]);
TetraminoType NextTetramino(TetraminoType tetraminoQueue[5]);
vec2i LocalToCell(vec2i local);
vec2i GetBlockCell(Tetramino tetramino, int blockIndex);

int ipow(int base, int exp);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Input input;

int playArea[PLAY_AREA_HEIGHT][PLAY_AREA_WIDTH] = {0};

#if DEBUG
int main()
#else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
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

	std::cout << X_OFFSET << "\n";

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

	DrawBuffer drawBuffer;
	drawBuffer.vao = quadVAO;
	drawBuffer.shader = shader;
	
	DrawBuffer texBuffer;
	texBuffer.vao = quadVAO;
	texBuffer.shader = texShader;

	//Load skins
	int skinIndex = 0;
	uint blockSkins[10];
	for (int i = 0; i < len(blockSkins); ++i)
	{
		char fileName[11];
		snprintf(fileName, sizeof(fileName), "skin%d.png", i);
		blockSkins[i] = loadTexture(fileName, true);
		if (!blockSkins[i]) blockSkins[i] = loadTexture("default_skin.png", true);
	}
	#if DEBUG
	uint debugSkin = loadTexture("test_block_skin4.png", true);
	#endif
	uint fontTexs[36] = {0};
	for (int i = 0; i < 36; ++i)
	{
		char fileName[6];
		char c = (char)((i < 26) ? i + 65 : NUM_TEX_OFFSET + i);
		snprintf(fileName, sizeof(fileName), "%c.png", c);
		fontTexs[i] = loadTexture(fileName, true);
	}

	bool playingGame = true;

	float elapsedTime = 0.0f;
	float stepTime = 1.0f;
	float heldTime = 0.0f;
	float moveThreshold = 0.125f;
	float tickTime = 0.0f;
	float moveTick = 0.0000000001f; //lol 
	int numTimesSamePos = 0;
	int score = 0;

	std::srand((int)time(nullptr));

	TetraminoType initialType = (TetraminoType)(std::rand() % 7 + 1);
	Tetramino currentTetramino = InitTetramino(initialType, vec2i_init(4, PLAY_AREA_Y_START+1));
	vec2i ghostTetraminoPos = GetGhostTetraminoPos(currentTetramino);

	bool changeTetramino = false;
	Tetramino heldTetramino = InitTetramino(NONE, vec2i_init(0));
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
			if (elapsedTime >= stepTime)
			{
				//Auto lock tetramino if been is same position 3 times, else move normally
				if (numTimesSamePos == 2)
				{
					currentTetramino.pos = ghostTetraminoPos;
					LockTetramino(currentTetramino, &score);
					nextTetraminoType = NextTetramino(tetraminoQueue);
					changeTetramino = true;
					numTimesSamePos = 0;
				}
				else
				{
					vec2i prevPos = currentTetramino.pos;
					if (!KeyPress(input.down)) MoveTetramino(&currentTetramino, VEC2I_DOWN);
					elapsedTime = 0.0f;

					if (currentTetramino.pos == prevPos)
					{
						numTimesSamePos++;
					}
					else
					{
						numTimesSamePos = 0;
						score += 10;
					}
				}
			}

			if (KeyDown(input.left))
			{
				heldTime = 0.0f;
				MoveTetramino(&currentTetramino, VEC2I_LEFT);
			}

			if (KeyDown(input.right))
			{
				heldTime = 0.0f;
				MoveTetramino(&currentTetramino, VEC2I_RIGHT);
			}

			if (KeyPress(input.left))
			{
				heldTime += deltaTime;
				if (heldTime > moveThreshold && tickTime > moveTick)
				{
					MoveTetramino(&currentTetramino, VEC2I_LEFT);
					tickTime = 0.0f;
				}
				else 
					tickTime += deltaTime;
				
			}

			if (KeyPress(input.right))
			{
				heldTime += deltaTime;
				if (heldTime > moveThreshold && tickTime > moveTick)
				{
					MoveTetramino(&currentTetramino, VEC2I_RIGHT);
					tickTime = 0.0f;
				}
				else 
					tickTime += deltaTime;
			}

			if (KeyPress(input.down))
			{
				vec2i prevPos = currentTetramino.pos;
				MoveTetramino(&currentTetramino, VEC2I_DOWN);
				if (currentTetramino.pos != prevPos)
					score += 10;
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
				heldTetramino = InitTetramino(currentTetramino.type, vec2i_init(0));
				nextTetraminoType = newType;
				changeTetramino = true;
				numTimesSamePos = 0;
			}

			if (KeyDown(input.space))
			{
				currentTetramino.pos = ghostTetraminoPos;
				LockTetramino(currentTetramino, &score);
				nextTetraminoType = NextTetramino(tetraminoQueue);
				changeTetramino = true;
				numTimesSamePos = 0;
			}
		}

		if (KeyDown(input.f4))
		{
			ResetGame(nextTetraminoType, &heldTetramino, tetraminoQueue, &score);
			playingGame = true;
			elapsedTime = 0.0f;
			numTimesSamePos = 0;
			changeTetramino = true;
			continue;
		}

		for (int i = 0; i < 10; ++i)
		{
			if (KeyDown(input.numbers[i]))
			{
				skinIndex = i;
				break;
			}
		}

		//Draw Black Backgorund
		for (int i = PLAY_AREA_Y_START; i < PLAY_AREA_HEIGHT; ++i)
		{
			for (int j = 0; j < PLAY_AREA_WIDTH; ++j)
			{
				vec2i pixelPos = WorldToPixel(vec2i_init(j, i));
				#if ENABLE_GRID
					DrawBorderedCell(drawBuffer, pixelPos, vec4_init(vec3_init(0.0f), 1.0f), 
									 vec4_init(vec3_init(0.15f), 1.0f));
				#else
					DrawCell(drawBuffer, pixelPos, vec4_init(vec3_init(0.0f), 1.0f));
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
					vec2i pixelPos = WorldToPixel(vec2i_init(j, i));
					DrawTexturedCell(texBuffer, blockSkins[skinIndex], pixelPos, 
										TetraminoColour((TetraminoType)playArea[i][j]));
				}
			}
		}

		//Draw tetramino
		if (!changeTetramino) //Flag here to prevent flashing of tetramino before clear
		{
			for (int i = 0; i < 4; ++i)
			{
				vec2i cell = GetBlockCell(currentTetramino, i);
				if (cell.y < PLAY_AREA_Y_START) continue;

				DrawTexturedCell(texBuffer, blockSkins[skinIndex], WorldToPixel(cell), 
								 TetraminoColour(currentTetramino.type));
			}
		}

		//Draw Ghost Tetramino
		if (!changeTetramino) //Flag here to prevent flashing of tetramino before clear
		{
			for (int i = 0; i < 4; ++i)
			{
				vec2i cell = ghostTetraminoPos + LocalToCell(currentTetramino.blockCoords[i]);
				if (cell.y < PLAY_AREA_Y_START) continue;
				
				vec4 ghostColour = TetraminoColour(currentTetramino.type);
				ghostColour.a = 0.5f;
				DrawTexturedCell(texBuffer, blockSkins[skinIndex], WorldToPixel(cell), ghostColour);
			}
		}

		//Draw Held Tetramino
		for (int i = 0; i < 4; ++i)
		{
			if (heldTetramino.type != NONE)
			{
				int x = heldTetramino.blockCoords[i].x - 3;
				int y = PLAY_AREA_Y_START - heldTetramino.blockCoords[i].y + 1;
				vec2i pixelPos = WorldToPixel(vec2i_init(x, y));
				DrawTexturedCell(texBuffer, blockSkins[skinIndex], pixelPos, 
								 TetraminoColour(heldTetramino.type));
			}
		}

		//Draw next tetraminos
		for (int i = 0; i < len(tetraminoQueue); ++i)
		{
			vec2i blocks[4];
			GetTetraminoBlocks(tetraminoQueue[i], blocks);
			int queueOffset = len(tetraminoQueue) - i - 1;
			for (int b = 0; b < 4; ++b)
			{
				vec2i cell = LocalToCell(blocks[b]);
				int x = PLAY_AREA_WIDTH + 3 + cell.x;
				int y =  3 * queueOffset + cell.y + PLAY_AREA_Y_START;
				vec2i pixelPos = WorldToPixel(vec2i_init(x, y));
				DrawTexturedCell(texBuffer, blockSkins[skinIndex], pixelPos, 
								 TetraminoColour(tetraminoQueue[i]));
			}
		}

		//If block goes over height, end the game
		for (int i = 0; i < len(playArea[PLAY_AREA_Y_START-1]); ++i)
		{
			if (playArea[PLAY_AREA_Y_START-1][i] != 0)
			{
				playingGame = false;
				break;
			}
		}

		//Draw Game Over Text
		if (!playingGame)
		{
			const char gameOver[10] = "GAME OVER";
			int y = SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH; 
			DrawText(texBuffer, fontTexs, gameOver, len(gameOver), vec2i_init(X_OFFSET, y), 
					 vec4_init(1.0f));
		}

		//Draw Restart Text
		{
			const char restartText[25] = "PRESS F4 TO RESTART GAME";
			int y = PLAY_AREA_HEIGHT + 3;
			int xOffset = ((SCR_WIDTH / CELL_PIXEL_LENGTH) - 24) / 2 ;
			vec2i pixelPos = WorldToPixel(vec2i_init(xOffset, y), false);
			DrawText(texBuffer, fontTexs, restartText, len(restartText) + 1, pixelPos, vec4_init(1.0f));
		}

		//Draw Score Text
		{
			const char scoreText[8] = "SCORE: ";
			vec2i pixelPos = WorldToPixel(vec2i_init(PLAY_AREA_WIDTH + 6, PLAY_AREA_Y_START), true);
			DrawText(texBuffer, fontTexs, scoreText, len(scoreText), pixelPos, vec4_init(1.0f));

			//Get num digits in score (int log10(score))
			if (score > 99999999) score = 99999999; //Overflow prevention
			int numDigits = 0;
			int tempScore = score;
			while (tempScore >= 1) 
			{
				tempScore /= 10;
				numDigits++;
			}

			tempScore = score;
			vec2i startPos = vec2i_init(PLAY_AREA_WIDTH + 6, PLAY_AREA_Y_START + 1);
			const int scoreLen = 8;
			for (int i = scoreLen - 1; i >= 0; --i)
			{
				char digit[2] = "0";
				if (i < numDigits)
				{
					int exp = ipow(10, i);
					int digitNum = tempScore / exp;
					digit[0] = (char)(digitNum + 48);
					tempScore -= digitNum * exp;
				}
				vec2i digitPixelPos = WorldToPixel(startPos + VEC2I_RIGHT * (scoreLen - 1- i));
				DrawText(texBuffer, fontTexs, digit, 2, digitPixelPos, vec4_init(1.0f));
			}
		}

		if (changeTetramino && playingGame)
		{
			currentTetramino = InitTetramino(nextTetraminoType, vec2i_init(4, PLAY_AREA_Y_START+1));
			
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

	for (int i = 0; i < 10; ++i)
		ProcessKey(window, &input.numbers[i], GLFW_KEY_0 + i);
}

void DrawCell(DrawBuffer buffer, vec2i pixelPos, vec4 colour)
{
    glUseProgram(buffer.shader);
	shader_SetVector4f(buffer.shader, "spriteColour", colour);

    glBindVertexArray(buffer.vao);
    glEnable(GL_SCISSOR_TEST);
    glScissor(pixelPos.x, pixelPos.y, CELL_PIXEL_LENGTH, CELL_PIXEL_LENGTH);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_SCISSOR_TEST);
}

void DrawBorderedCell(DrawBuffer buffer, vec2i pixelPos, vec4 fillColour, vec4 borderColour)
{
	glEnable(GL_SCISSOR_TEST);
	glUseProgram(buffer.shader);
	
	//Draw border
	shader_SetVector4f(buffer.shader, "spriteColour", borderColour);
    glBindVertexArray(buffer.vao);
	glScissor(pixelPos.x, pixelPos.y, CELL_PIXEL_LENGTH, CELL_PIXEL_LENGTH);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//Draw centre
	shader_SetVector4f(buffer.shader, "spriteColour", fillColour);
    glScissor(pixelPos.x + 1, pixelPos.y + 1, CELL_PIXEL_LENGTH - 2, CELL_PIXEL_LENGTH - 2);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_SCISSOR_TEST);
}

void DrawTexturedCell(DrawBuffer texBuffer, uint texture, vec2i pixelPos, vec4 colour)
{
	glUseProgram(texBuffer.shader);
	shader_SetVector4f(texBuffer.shader, "spriteColour", colour);
	shader_SetInt(texBuffer.shader, "tex", 0);

	glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(texBuffer.vao);
    glEnable(GL_SCISSOR_TEST);
    glScissor(pixelPos.x, pixelPos.y, CELL_PIXEL_LENGTH, CELL_PIXEL_LENGTH);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_SCISSOR_TEST);
}

void DrawText(DrawBuffer texBuffer, uint fontTexs[36], 
			  const char* text, int textLength, vec2i pixelPos, vec4 colour)
{
	for (int i = 0; i < textLength; ++i)
	{
		if (text[i] == ' ') continue;

		int x = pixelPos.x + CELL_PIXEL_LENGTH * i;

		int charIndex = (int)((text[i] >= 'A' && text[i] <= 'Z') ? 
							text[i] - 65 : text[i] - NUM_TEX_OFFSET);
		if (charIndex >= 0 && charIndex < 36)	
			DrawTexturedCell(texBuffer, fontTexs[charIndex], vec2i_init(x, pixelPos.y), colour);
	}
}

vec2i WorldToPixel(vec2i localPos, bool applyXOffset)
{
	int x = X_OFFSET * (applyXOffset) + CELL_PIXEL_LENGTH * localPos.x;
	int y = SCR_HEIGHT - Y_OFFSET - CELL_PIXEL_LENGTH * localPos.y;
	return vec2i_init(x, y);
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
			   TetraminoType tetraminoQueue[5],
			   int* score)
{
	nextTetraminoType = (TetraminoType)(std::rand() % 7 + 1);
	*heldTetramino = InitTetramino(NONE, vec2i_init(0));
	
	for (int i = 0; i < 5; ++i)
		tetraminoQueue[i] =  (TetraminoType)(std::rand() % 7 + 1);
	
	for (int i = 0; i < PLAY_AREA_HEIGHT; ++i)
		for (int j = 0; j < PLAY_AREA_WIDTH; ++j)
			playArea[i][j] = 0;

	*score = 0;
}

vec4 TetraminoColour(TetraminoType tetraminoType)
{
	switch (tetraminoType)
	{
		default:       return vec4_init(vec3_init(0.0f), 1.0f);
		case SQUARE:   return vec4_init(1.0f, 1.0f, 0.0f, 1.0f);
		case LINE:     return vec4_init(0.0f, 1.0f, 1.0f, 1.0f);
		case L_BLOCK:  return vec4_init(1.0f, 153.0f/255.0f, 0.0f, 1.0f);
		case RL_BLOCK: return vec4_init(0.0f, 0.0f, 1.0f, 1.0f);
		case S_BLOCK:  return vec4_init(0.0f, 1.0f, 0.0f, 1.0f);
		case RS_BLOCK: return vec4_init(1.0f, 0.0f, 0.0f, 1.0f);
		case T_BLOCK:  return vec4_init(153.0f/255.0f, 51.0f/255.0f, 1.0f, 1.0f);
	}
}

//Places tetramino blocks into play area
void LockTetramino(Tetramino tetramino, int* score)
{
	for (int i = 0; i < 4; i++)
	{
		vec2i cell = GetBlockCell(tetramino, i);
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
	ClearLines(lowestY, highestY, score);
}

void ClearLines(int lowestRow, int highestRow, int* score)
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
			*score += 100;
		} 
	} 
}

void MoveTetramino(Tetramino* tetramino, vec2i direction)
{	
	for (int i = 0; i < 4; ++i)
	{
		vec2i newCell = GetBlockCell(*tetramino, i) + direction;

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
	vec2i rotatedBlocks[4];
	for (int i = 0; i < 4; ++i)
	{
		vec2i rot; 
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
	vec2i offset = vec2i_init(0);
	for (int i = 0; i < 4; ++i)
	{
		vec2i cell = tetramino->pos + LocalToCell(rotatedBlocks[i]);

		int rDiff = cell.x - (PLAY_AREA_WIDTH - 1);
		int lDiff = cell.x - 0;
		int yDiff = cell.y - (PLAY_AREA_HEIGHT - 1);
		if (rDiff > offset.x) offset.x = rDiff;
		if (lDiff < offset.x) offset.x = lDiff;
		if (yDiff > offset.y) offset.y = yDiff;
	}
	tetramino->pos -= offset;

	//If rotation would intersect other block, apply wall kick
	vec2i kickDirections[5] = {vec2i_init(0), VEC2I_LEFT, VEC2I_RIGHT, VEC2I_UP, VEC2I_DOWN};
	int k = 0;
	while (k < 5)
	{
		int numFit = 0;
		for (int i = 0; i < 4; ++i)
		{
			vec2i newCell = tetramino->pos + LocalToCell(rotatedBlocks[i]) + kickDirections[k];
			
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

vec2i GetGhostTetraminoPos(Tetramino tetramino)
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
			vec2i cell = GetBlockCell(tetramino, i);
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
	return tetramino.pos + VEC2I_DOWN * (y - 1);
}

Tetramino InitTetramino(TetraminoType type, vec2i pos)
{
	Tetramino tetramino;
	tetramino.type = type;
	tetramino.pos = pos;
	GetTetraminoBlocks(type, tetramino.blockCoords);
	return tetramino;
}

void GetTetraminoBlocks(TetraminoType type, vec2i blockCoords[4])
{
	//All the "blockCoords" are relative to their centre except the square and line are 
	//relative to their centre. Square is relative to top left and line is relative to
	//third block
	switch (type)
	{
		default:
			for (int i = 0; i < 4; ++i) 
				blockCoords[i] = vec2i_init(0);
			break;
		case SQUARE:   
			blockCoords[0] = vec2i_init( 0,  0);
			blockCoords[1] = vec2i_init( 1,  0);
			blockCoords[2] = vec2i_init( 0,  1);
			blockCoords[3] = vec2i_init( 1,  1);
			break;
		case LINE:
			blockCoords[0] = vec2i_init(-2,  0);
			blockCoords[1] = vec2i_init(-1,  0);
			blockCoords[2] = vec2i_init( 0,  0);
			blockCoords[3] = vec2i_init( 1,  0);
			break;
		case L_BLOCK:  
			blockCoords[0] = vec2i_init( 1,  1);
			blockCoords[1] = vec2i_init( 1,  0);
			blockCoords[2] = vec2i_init( 0,  0);
			blockCoords[3] = vec2i_init(-1,  0);
			break;
		case RL_BLOCK: 
			blockCoords[0] = vec2i_init(-1,  1);
			blockCoords[1] = vec2i_init(-1,  0);
			blockCoords[2] = vec2i_init( 0,  0);
			blockCoords[3] = vec2i_init( 1,  0);
			break;
		case S_BLOCK: 
			blockCoords[0] = vec2i_init(-1,  0);
			blockCoords[1] = vec2i_init( 0,  0);
			blockCoords[2] = vec2i_init( 0,  1);
			blockCoords[3] = vec2i_init( 1,  1);
			break;
		case RS_BLOCK: 
			blockCoords[0] = vec2i_init(-1,  1);
			blockCoords[1] = vec2i_init( 0,  1);
			blockCoords[2] = vec2i_init( 0,  0);
			blockCoords[3] = vec2i_init( 1,  0);
			break;
		case T_BLOCK:  
			blockCoords[0] = vec2i_init(-1,  0);
			blockCoords[1] = vec2i_init( 0,  0);
			blockCoords[2] = vec2i_init( 0,  1);
			blockCoords[3] = vec2i_init( 1,  0);
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

vec2i LocalToCell(vec2i local)
{
	return vec2i_init(local.x, -local.y);
}

vec2i GetBlockCell(Tetramino tetramino, int blockIndex)
{
	return tetramino.pos + LocalToCell(tetramino.blockCoords[blockIndex]);
}

int ipow(int base, int exp)
{
	int result = 1;
	for (int i = 0; i < exp; ++i)
		result *= base;
	return result;
}
