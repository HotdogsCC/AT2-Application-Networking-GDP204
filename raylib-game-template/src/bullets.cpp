#include "bullets.h"
#include "networking.h"
#include "vector"

class Bullet
{
public:
	Bullet();

	//where the bullet is
	Vector2 position;
	
	//the direction in which this bullet should travel
	bool shouldTravelRight;
};

Bullet::Bullet()
{
	position.x = -999;
	position.y = -999;
	shouldTravelRight = false;
}

//memory pool of all bullets in the scene
Bullet* bullets[BULLET_POOL_SIZE];

//used as a temp buffer for bullets that just got destroyed on server and clients need to be alerted
bool bulletsToBeKilled[BULLET_POOL_SIZE]; 

//checks if the given index has a valid bullet in the memory pool
bool BulletIsValid(int index)
{
	//is this out of range?
	if (index >= BULLET_POOL_SIZE) return false;

	//is this a null pointer
	if (bullets[index] == nullptr) return false;

	//is this a 'null' vector?
	return bullets[index]->position.x != -999 || bullets[index]->position.y != -999;
}

//checks if the bullet was destroyed this frame; used for server sending destroy commands to clients
bool BulletJustDied(int index)
{
	//is this out of range?
	if (index >= BULLET_POOL_SIZE) return false;

	return bulletsToBeKilled[index];
}

//used once a destroy command was sent to clients
void ResetBulletDiedStatus(int index)
{
	bulletsToBeKilled[index] = false;
}

//creates the bullet memory pool
void InitBullets()
{
	//null out each piece of memory
	for (int i = 0; i < BULLET_POOL_SIZE; i++)
	{
		bullets[i] = nullptr;
		bulletsToBeKilled[i] = false;
	}
}

//spawns a bullet server side
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
				//make the bullet in the empty slot
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

//adds a bullet client side; runs when a message is receieved from server
void AddBulletToArray(int id, int posX, int posY)
{
	//if the vector is 'null', delete it from the pool
	if (posX == -999 && posY == -999)
	{
		delete bullets[id];
		bullets[id] = nullptr;
		return;
	}

	//if the slot is currently empty, make a new slot for it
	if (bullets[id] == nullptr)
	{
		bullets[id] = new Bullet();
	}

	//set the position of the bullet
	bullets[id]->position = { static_cast<float>(posX), static_cast<float>(posY) };
	return;
}

//removes a bullet client side; runs when a message is receieved from server
void RemoveBulletFromArray(int id)
{
	delete bullets[id];
	bullets[id] = nullptr;
}

//returns the position of a given bullet index
Vector2 GetBulletPosition(int index)
{
	//if the bullet isnt valid, return a 'null' vector
	if (!BulletIsValid(index))
	{
		return { -999, -999 };
	}

	//otherwise return the actual position
	return bullets[index]->position;
}

//runs every frame to move bullets
void UpdateBullets(float deltaTime)
{
	//for each bullet in our pool
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
						//construct bounds around the bullet
						Rectangle bulletRect;
						bulletRect.x = bullets[i]->position.x;
						bulletRect.y = bullets[i]->position.y;
						bulletRect.width = bulletWidth;
						bulletRect.height = bulletHeight;

						//construct bounds around the player
						Rectangle playerRect;
						playerRect.x = itClientPos.x;
						playerRect.y = itClientPos.y;
						playerRect.width = PLAYER_CHARACTER_SIZE;
						playerRect.height = PLAYER_CHARACTER_SIZE;

						//are they colliding?
						if (CheckCollisionRecs(bulletRect, playerRect))
						{
							//destroy the bullet
							bulletsToBeKilled[i] = true;
							delete bullets[i];
							bullets[i] = nullptr;

							PlaySound(splat);

							//move on to the next bullet
							break;
						}

					}
				}
			}
			
		}
	}
}