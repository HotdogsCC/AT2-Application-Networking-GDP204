#pragma once

#include "raylib.h"
#define BULLET_POOL_SIZE 64
#define PLAYER_CHARACTER_SIZE 20

#ifdef __cplusplus
extern "C" {
#endif
	const int bulletWidth = 10;
	const int bulletHeight = 5;
	const float bulletSpeed = 600.0f;

	void InitBullets();

	void CreateBullet(Vector2, bool);

	//used by client
	void AddBulletToArray(int id, int posX, int posY);
	void RemoveBulletFromArray(int id);

	bool BulletIsValid(int index);
	bool BulletJustDied(int index);

	void ResetBulletDiedStatus(int index);


	//returns the position of a given bullet index
	Vector2 GetBulletPosition(int);

	void UpdateBullets(float);

#ifdef __cplusplus
}
#endif
