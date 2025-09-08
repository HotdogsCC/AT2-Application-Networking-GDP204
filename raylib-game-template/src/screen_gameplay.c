/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
*
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "networking.h"
#include "math.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

//player
int playerSize = 20;
Vector2 position = { 0, 0 };
Vector2 velocity = { 0, 0 };
int moveSpeed = 5;
float gravity = 10;
float jumpForce = 10;


//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    // TODO: Initialize GAMEPLAY screen variables here!
    framesCounter = 0;
    finishScreen = 0;

    position.x = GetScreenWidth() / 2;
    position.y = GetScreenHeight() / 2;

    jumpForce = gravity;
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    //take user input for player positions
    if (IsKeyDown(KEY_RIGHT))
    {
        position.x += moveSpeed;
    }
    if (IsKeyDown(KEY_LEFT))
    {
        position.x -= moveSpeed;
    }
    if (IsKeyPressed(KEY_UP))
    {
        velocity.y = -jumpForce;
    }
    if (IsKeyDown(KEY_DOWN))
    {
       // position.y += moveSpeed;
    }

    position.y += velocity.y;

    if (position.y >= GetScreenHeight() - playerSize)
    {
        velocity.y = 0.0f;
        position.y = GetScreenHeight() - playerSize;
    }
    else
    {
        velocity.y += GetFrameTime() * gravity;

    }

    if (position.y <= 0)
    {
        velocity.y = -velocity.y;
        position.y += 10;
    }

    if (position.x <= 0) position.x = 0;
    if (position.x >= GetScreenWidth() - playerSize) position.x = GetScreenWidth() - playerSize;

    UpdatePacketPosition(position.x, position.y);

}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // TODO: Draw GAMEPLAY screen here!
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), PURPLE);
    Vector2 pos = { 20, 10 };
    DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize*3.0f, 4, MAROON);
    DrawText("LEFT/RIGHT TO MOVE | UP TO JUMP | ESC TO CLOSE", 130, 220, 20, MAROON);

    //draw clients (up to 8 players)
    for (int i = 0; i < 8; i++)
    {
        Vector2Int itClientPos = GetClientPosition(i);
        if (Vector2Int_IsValid(itClientPos) && i != GetMyID())
        {
            DrawRectangle(itClientPos.x, itClientPos.y, playerSize, playerSize, GREEN);
            const char* clientNick = GetClientNickname(i);
            char clientNickWithTerminator[9];
            for (int i = 0; i < 8; i++)
            {
                clientNickWithTerminator[i] = clientNick[i];
            }
            clientNickWithTerminator[8] = '\0';
            DrawText(clientNickWithTerminator, itClientPos.x, itClientPos.y - 20, 20, WHITE);
        }
    }

    //draw this player
    DrawRectangle(position.x, position.y, playerSize, playerSize, RED);
    DrawText(GetNickname(), position.x, position.y - 20, 20, WHITE);

    

}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}