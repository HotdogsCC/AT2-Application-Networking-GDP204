#include "bullets.h"
#include "networking.h"

class Bullet
{
public:
	Bullet();

	Vector2 position;
	bool shouldTravelRight;
};

Bullet::Bullet()
{
	position.x = -999;
	position.y = -999;
	shouldTravelRight = false;
}

Bullet* bullets[BULLET_POOL_SIZE];

bool BulletIsValid(int index)
{
	if (index >= BULLET_POOL_SIZE) return false;

	if (bullets[index] == nullptr) return false;

	return bullets[index]->position.x != -999 || bullets[index]->position.y != -999;
}

void InitBullets()
{
	for (int i = 0; i < BULLET_POOL_SIZE; i++)
	{
		bullets[i] = nullptr;
	}
}

void CreateBullet(Vector2 inPos, bool inShouldTravelRight)
{
	//make the bullet if we are the server
	if (IsServer())
	{
		for (int i = 0; i < BULLET_POOL_SIZE; i++)
		{
			//look for an empty slot
			if (bullets[i] == nullptr)
			{
				bullets[i] = new Bullet();
				bullets[i]->position = inPos;
				bullets[i]->shouldTravelRight = inShouldTravelRight;
				return;
			}
		}
	}
	//otherwise, ask the server to make a bullet for us
	else
	{
		//RequestBu
	}

	
}

void AddBulletToArray(int id, int posX, int posY)
{
	//if it is null, delete
	if (posX == -999 && posY == -999)
	{
		delete bullets[id];
		bullets[id] = nullptr;
		return;
	}

	if (bullets[id] == nullptr)
	{
		bullets[id] = new Bullet();
	}

	bullets[id]->position = { static_cast<float>(posX), static_cast<float>(posY) };
	return;
}

Vector2 GetBulletPosition(int index)
{
	if (!BulletIsValid(index))
	{
		return { -999, -999 };
	}

	return bullets[index]->position;
}

void UpdateBullets(float deltaTime)
{
	for (int i = 0; i < BULLET_POOL_SIZE; i++)
	{
		if (BulletIsValid(i))
		{
			//move the bullet
			if (bullets[i]->shouldTravelRight) bullets[i]->position.x += bulletSpeed * deltaTime;
			else bullets[i]->position.x -= bulletSpeed * deltaTime;

			//kill the bullet if it goes out of bounds
			if (bullets[i]->position.x <= 0 || bullets[i]->position.x >= GetScreenWidth() - bulletWidth)
			{
				delete bullets[i];
				bullets[i] = nullptr;
			}
			
		}
	}
}