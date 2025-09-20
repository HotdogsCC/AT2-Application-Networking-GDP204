#pragma once

#include "raylib.h"
#define BULLET_POOL_SIZE 64

#ifdef __cplusplus
extern "C" {
#endif
	const int bulletWidth = 10;
	const int bulletHeight = 5;
	const float bulletSpeed = 600.0f;

	void InitBullets();

	void CreateBullet(Vector2, bool);

	bool BulletIsValid(int index);

	//returns the position of a given bullet index
	Vector2 GetBulletPosition(int);

	void UpdateBullets(float);

#ifdef __cplusplus
}
#endif
