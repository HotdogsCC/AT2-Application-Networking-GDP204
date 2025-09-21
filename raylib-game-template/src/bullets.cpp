#include "bullets.h"
#include "networking.h"
#include "vector"



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
bool bulletsToBeKilled[BULLET_POOL_SIZE]; //used as a queue for network events

bool BulletIsValid(int index)
{
	if (index >= BULLET_POOL_SIZE) return false;

	if (bullets[index] == nullptr) return false;

	return bullets[index]->position.x != -999 || bullets[index]->position.y != -999;
}

bool BulletJustDied(int index)
{
	if (index >= BULLET_POOL_SIZE) return false;

	return bulletsToBeKilled[index];
}

void ResetBulletDiedStatus(int index)
{
	bulletsToBeKilled[index] = false;
}

void InitBullets()
{
	for (int i = 0; i < BULLET_POOL_SIZE; i++)
	{
		bullets[i] = nullptr;
		bulletsToBeKilled[i] = false;
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
		CreateBulletOnServer(static_cast<int>(inPos.x), static_cast<int>(inPos.y), inShouldTravelRight);
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

void RemoveBulletFromArray(int id)
{
	delete bullets[id];
	bullets[id] = nullptr;
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
				bulletsToBeKilled[i] = true;
				delete bullets[i];
				bullets[i] = nullptr;
			}

			//check for player collisions
			else
			{
				//check if we are colliding with any players
				for (char j = 0; j < 16; j++)
				{
					Vector2Int itClientPos = GetClientPosition(j);

					//ignore if it isnt valid
					if (Vector2Int_IsValid(itClientPos))
					{
						Rectangle bulletRect;
						bulletRect.x = bullets[i]->position.x;
						bulletRect.y = bullets[i]->position.y;
						bulletRect.width = bulletWidth;
						bulletRect.height = bulletHeight;

						Rectangle playerRect;
						playerRect.x = itClientPos.x;
						playerRect.y = itClientPos.y;
						playerRect.width = PLAYER_CHARACTER_SIZE;
						playerRect.height = PLAYER_CHARACTER_SIZE;

						//are we colliding?
						if (CheckCollisionRecs(bulletRect, playerRect))
						{
							bulletsToBeKilled[i] = true;
							delete bullets[i];
							bullets[i] = nullptr;

							PlaySound(splat);

							break;
						}

					}
				}
			}
			
		}
	}
}