#pragma once

#include "raylib.h"
#define BULLET_POOL_SIZE 64 //the amount of bullets that could exist at one time
#define PLAYER_CHARACTER_SIZE 20 //the width/height in pixels of the player square

#ifdef __cplusplus
extern "C" {
#endif
	//the sound that plays when a bullet hits a player
	extern Sound splat;

	//bullet consts
	const int bulletWidth = 10;
	const int bulletHeight = 5;
	const float bulletSpeed = 600.0f;

	//creates the bullet memory pool
	void InitBullets();

	//spawns a bullet server side
	void CreateBullet(Vector2, bool);

	//adds a bullet client side; runs when a message is receieved from server
	void AddBulletToArray(int id, int posX, int posY);
	//removes a bullet client side; runs when a message is receieved from server
	void RemoveBulletFromArray(int id);

	//checks if the given index has a valid bullet in the memory pool
	bool BulletIsValid(int index);
	//checks if the bullet was destroyed this frame; used for server sending destroy commands to clients
	bool BulletJustDied(int index);
	//used once a destroy command was sent to clients
	void ResetBulletDiedStatus(int index);


	//returns the position of a given bullet index
	Vector2 GetBulletPosition(int);

	//runs every frame to move bullets
	void UpdateBullets(float);

#ifdef __cplusplus
}
#endif
