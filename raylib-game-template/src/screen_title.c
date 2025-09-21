/**********************************************************************************************
*
*   raylib - Advance Game template
*
*   Title Screen Functions Definitions (Init, Update, Draw, Unload)
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

#include "networking.h"
#include "raylib.h"
#include "screens.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

static float buttonsDistance = 200;
static int textFontSize = 40;

bool serverIsHovered = false;
bool clientIsHovered = false;

#define MAX_INPUT_CHARS 8
char inputNickname[MAX_INPUT_CHARS + 1] = "Player\0";
int letterCount = 6;

Rectangle serverButtonRect;
Rectangle clientButtonRect;



//----------------------------------------------------------------------------------
// Title Screen Functions Definition
//----------------------------------------------------------------------------------

// Title Screen Initialization logic
void InitTitleScreen(void)
{
    //button set up
    const float buttonWidth = 200;
    const float buttonHeight = 150;
    const float buttonY = (GetScreenHeight() / 2) - (buttonHeight / 2);

    //server button set up
    const float serverButtonX = (GetScreenWidth() / 2) - (buttonWidth / 2) - buttonsDistance;
    serverButtonRect.x = serverButtonX;
    serverButtonRect.y = buttonY;
    serverButtonRect.width = buttonWidth;
    serverButtonRect.height = buttonHeight;

    //client button set up
    const float clientButtonX = (GetScreenWidth() / 2) - (buttonWidth / 2) + buttonsDistance;
    clientButtonRect.x = clientButtonX;
    clientButtonRect.y = buttonY;
    clientButtonRect.width = buttonWidth;
    clientButtonRect.height = buttonHeight;
}

// Title Screen Update logic
void UpdateTitleScreen(void)
{
    // Press enter or tap to change to GAMEPLAY screen
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        if (serverIsHovered)
        {
            serverIsHovered = false;
            SetNickname(inputNickname);
            StartServer();
            finishScreen = 2; //GAMEPLAY
            PlaySound(fxCoin);
        }
        if (clientIsHovered)
        {
            clientIsHovered = false;
            SetNickname(inputNickname);
            StartClient();
            finishScreen = 2; //GAMEPLAY
            PlaySound(fxCoin);
        }
        
    }

    //is the mouse hovering over the button?
    serverIsHovered = CheckCollisionPointRec(GetMousePosition(), serverButtonRect);
    clientIsHovered = CheckCollisionPointRec(GetMousePosition(), clientButtonRect);

    //read keys
    int key = GetCharPressed();
    while (key > 0)
    {
        // NOTE: Only allow keys in range [32..125]
        if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT_CHARS))
        {
            inputNickname[letterCount] = (char)key;
            inputNickname[letterCount + 1] = '\0'; // Add null terminator at the end of the string
            letterCount++;
        }

        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        letterCount--;
        if (letterCount < 0) letterCount = 0;
        inputNickname[letterCount] = '\0';
    }

}

// Title Screen Draw logic
void DrawTitleScreen(void)
{
    // TODO: Draw TITLE screen here!
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), GREEN);
    const Vector2 pos = { 20, 10 };
    DrawTextEx(font, "CHARLIE CAMPBELL NETWORKING ASSESSMENT 2", pos, font.baseSize*3.0f, 4, DARKGREEN);

    //draws server and client buttons
    if (serverIsHovered) DrawRectangleRec(serverButtonRect, LIGHTGRAY);
    else DrawRectangleRec(serverButtonRect, GRAY);
    DrawText("Server", serverButtonRect.x, serverButtonRect.y, textFontSize, WHITE);

    if (clientIsHovered) DrawRectangleRec(clientButtonRect, LIGHTGRAY);
    else DrawRectangleRec(clientButtonRect, GRAY);
    DrawText("Client", clientButtonRect.x, clientButtonRect.y, textFontSize, WHITE);

    //draw nickname
    DrawText(inputNickname, GetScreenWidth() / 2 - 90, GetScreenHeight() - 100, 40, WHITE);
    DrawText("Enter Your Nickname", GetScreenWidth() / 2 - 90, GetScreenHeight() - 50, 20, DARKGREEN);
}

// Title Screen Unload logic
void UnloadTitleScreen(void)
{
    // TODO: Unload TITLE screen variables here!
}

// Title Screen should finish?
int FinishTitleScreen(void)
{
    return finishScreen;
}