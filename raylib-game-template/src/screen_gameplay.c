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
#include "bullets.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

//player
int playerSize = 20;
Vector2 position = { 0, 0 };
Vector2 velocity = { 0, 0 };
int moveSpeed = 400;
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
    if(IsServer) UpdateBullets(GetFrameTime());
   

    //bullet input
    if (IsKeyPressed(KEY_SPACE))
    {
        if (IsKeyDown(KEY_LEFT))
        {
            CreateBullet(position, false);
        }
        else
        {
            Vector2 spawnPos;
            spawnPos.x = position.x + playerSize;
            spawnPos.y = position.y;
            CreateBullet(spawnPos, true);
        }
        
    }

    //take user input for player positions
    if (IsKeyDown(KEY_RIGHT))
    {
        position.x += moveSpeed * GetFrameTime();
    }
    if (IsKeyDown(KEY_LEFT))
    {
        position.x -= moveSpeed * GetFrameTime();
    }
    if (IsKeyPressed(KEY_UP))
    {
        velocity.y = -jumpForce;
    }

    //apply velocity
    position.y += velocity.y;

    //ground logic
    if (position.y >= GetScreenHeight() - playerSize)
    {
        velocity.y = 0.0f;
        position.y = GetScreenHeight() - playerSize;
    }
    else
    {
        velocity.y += GetFrameTime() * gravity;

    }

    //rebounds off ceiling
    if (position.y <= 0)
    {
        velocity.y = -velocity.y;
        position.y += 10;
    }

    //confine player within the width of the screen
    if (position.x <= 0) position.x = 0;
    if (position.x >= GetScreenWidth() - playerSize) position.x = GetScreenWidth() - playerSize;

    //send our position to the server
    UpdatePacketPosition(position.x, position.y);

}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    //draw background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), PURPLE);
    Vector2 pos = { 20, 10 };
    DrawTextEx(font, "GAMEPLAY SCREEN", pos, font.baseSize*3.0f, 4, MAROON);
    DrawText("LEFT/RIGHT TO MOVE | UP TO JUMP | ESC TO CLOSE", 130, 220, 20, MAROON);

    //draw bullets
    for (int i = 0; i < BULLET_POOL_SIZE; i++)
    {
        if (BulletIsValid(i))
        {
            Vector2 bulletPos = GetBulletPosition(i);
            DrawRectangle(bulletPos.x, bulletPos.y, bulletWidth, bulletHeight, BLACK);
        }
    }

    //draw clients
    for (char i = 0; i < 16; i++)
    {
        Vector2Int itClientPos = GetClientPosition(i);
        //ignore if it is us
        if (Vector2Int_IsValid(itClientPos) && i != GetMyID())
        {
            //draw player box
            DrawRectangle(itClientPos.x, itClientPos.y, playerSize, playerSize, GREEN);

            //draw player name
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