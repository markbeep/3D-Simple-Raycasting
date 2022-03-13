#include "Platform/Platform.hpp"
#include <cmath>
#include <iostream>

const int TOTAL_RANDOM_WALLS = 0;
const int FOV = 90;
const int ACCURACY = 3;
const float MOVESPEED = 0.03;
const float TURNSPEED = 0.03;

struct playerStruct
{
	sf::Vector2f pos;
	sf::Vector2f dir;
	sf::Vector2f* rays;
};

sf::Vector2f* castSingle(sf::Vector2f pos, sf::Vector2f rayDir, sf::Vertex wall[2]);
void setDir(playerStruct* player, sf::Vector2f dir);
float calcPerpDistance(sf::Vector2f a, sf::Vector2f b);
float lazyCalculateDistance(sf::Vector2f a, sf::Vector2f b);
sf::Vector2f* castAll(sf::Vector2f pos, sf::Vector2f rayDir, sf::Vertex walls[][2], int n);
void viewDir(sf::Vector2f* rays, float dirDeg);
void addBox(sf::Vector2f pos, sf::Vertex walls[][2], int index, int size);

int main()
{
	util::Platform platform;

	sf::RenderWindow window;
	// in Windows at least, this must be called before creating the window
	float screenScalingFactor = platform.getScreenScalingFactor(window.getSystemHandle());
	// Use the screenScalingFactor
	window.create(sf::VideoMode(400.0f * screenScalingFactor, 800.0f * screenScalingFactor), "Raytracing Practice");
	platform.setIcon(window.getSystemHandle());
	sf::Vector2u size = window.getSize();

	sf::Event event;

	const int TOTAL_BOXES = 4;
	const int TOTAL_WALLS = TOTAL_RANDOM_WALLS + 4 + TOTAL_BOXES * 4;
	// draw 4 walls surrounding the field
	sf::Vertex walls[TOTAL_WALLS][2] = {
		{ sf::Vertex(sf::Vector2f(0, 0)), sf::Vertex(sf::Vector2f(0, size.y)) },
		{ sf::Vertex(sf::Vector2f(0, size.y)), sf::Vertex(sf::Vector2f(size.x, size.y)) },
		{ sf::Vertex(sf::Vector2f(size.x, 0)), sf::Vertex(sf::Vector2f(size.x, size.y)) },
		{ sf::Vertex(sf::Vector2f(0, 0)), sf::Vertex(sf::Vector2f(size.x, 0)) }
	};
	// randomly draw walls
	for (int i = 0; i < TOTAL_RANDOM_WALLS; i++)
	{
		walls[i + 4][0] = sf::Vertex(sf::Vector2f(1. * rand() / RAND_MAX * size.x, 1. * rand() / RAND_MAX * 400 + 400));
		walls[i + 4][1] = sf::Vertex(sf::Vector2f(1. * rand() / RAND_MAX * size.x, 1. * rand() / RAND_MAX * 400 + 400));
	}
	// draw boxes
	addBox(sf::Vector2f(50, 500), walls, TOTAL_RANDOM_WALLS + 4, 50);
	addBox(sf::Vector2f(300, 500), walls, TOTAL_RANDOM_WALLS + 8, 50);
	addBox(sf::Vector2f(200, 550), walls, TOTAL_RANDOM_WALLS + 12, 50);
	addBox(sf::Vector2f(30, 300), walls, TOTAL_RANDOM_WALLS + 16, 50);

	sf::Vector2f arr[FOV * ACCURACY];
	playerStruct player = { sf::Vector2f(200, 600), sf::Vector2f(0, 1), arr };

	float viewDirection = 0;

	sf::CircleShape viewPoint(5);
	viewPoint.setOrigin(sf::Vector2f(5, 5));

	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// handle movement
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			player.pos.x -= MOVESPEED;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			player.pos.x += MOVESPEED;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			player.pos.y -= MOVESPEED;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			player.pos.y += MOVESPEED;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			viewDirection -= TURNSPEED;
			if (viewDirection < 0)
				viewDirection = 360;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			viewDirection += TURNSPEED;
			if (viewDirection > 360)
				viewDirection = 0;
		}

		window.clear();

		viewDir(arr, viewDirection);
		// setDir(&playerStruct, sf::Vector2f(sf::Mouse::getPosition(window)));
		for (int i = 0; i < FOV * ACCURACY; i++)
		{

			sf::Vector2f* point = castAll(player.pos, player.rays[i], walls, TOTAL_WALLS);
			if (point != NULL)
			{
				// viewPoint.setPosition(*point);
				// window.draw(viewPoint);
				// sf::Vertex ray[] = { sf::Vertex(player.pos), sf::Vertex(*point) };
				// ray->color.a = 20;
				// ray[0].color = sf::Color(255, 255, 255, 50);
				// ray[1].color = sf::Color(255, 255, 255, 50);
				// window.draw(ray, 2, sf::Lines);

				float dist = calcPerpDistance(player.pos, *point);
				int height = size.y;
				int drawHeight = height / dist * 10; // screen height / dist
				int drawStart = -drawHeight / 2 + height / 2;
				if (drawStart < 0)
					drawStart = 0;

				float width = 1.0 * size.x / (FOV * ACCURACY);
				sf::RectangleShape rect(sf::Vector2f(width, drawHeight));
				// rect.setOrigin(sf::Vector2f(0, drawHeight / 2));
				rect.setPosition(sf::Vector2f(i * width, drawStart));
				int color = 255 / dist * 50;
				if (color > 255)
					color = 255;
				if (color < 0)
					color = 0;
				rect.setFillColor(sf::Color(color, color, color, 255));
				window.draw(rect);
			}
		}

		sf::CircleShape circ(10);
		circ.setOrigin(sf::Vector2f(10, 10));
		circ.setPosition(player.pos);
		window.draw(circ);

		// draws the rays going out from the player
		for (int i = 0; i < FOV * ACCURACY; i++)
		{
			sf::Vertex ray[] = { sf::Vertex(player.pos), sf::Vertex(player.pos + 50.0f * player.rays[i]) };
			ray->color.a = 20;
			ray[0].color = sf::Color(255, 255, 255, 255);
			ray[1].color = sf::Color(255, 255, 255, 0);
			window.draw(ray, 2, sf::Lines);
		}

		// draws the walls
		for (int i = 0; i < TOTAL_WALLS; i++)
		{
			window.draw(walls[i], 2, sf::Lines);
		}
		window.display();
	}

	return 0;
}

/**
 * @brief Calculates the distance between two vectors without square-rooting the result
 */
float lazyCalculateDistance(sf::Vector2f a, sf::Vector2f b)
{
	float x = a.x - b.x;
	float y = a.y - b.y;
	return x * x + y * y;
}

float calcPerpDistance(sf::Vector2f a, sf::Vector2f b)
{
	return sqrt(lazyCalculateDistance(a, b));
	// return cos(atan2(a.y, a.x) - atan2(b.y, b.x)) * dist;
}

sf::Vector2f* castAll(sf::Vector2f pos, sf::Vector2f rayDir, sf::Vertex walls[][2], int n)
{
	float minDistance = INFINITY;
	sf::Vector2f* closestPoint = NULL;
	float tmp;
	for (int i = 0; i < n; i++)
	{
		sf::Vector2f* p = castSingle(pos, rayDir, walls[i]);
		if (p == NULL)
			continue;
		if ((tmp = lazyCalculateDistance(pos, *p)) < minDistance)
		{
			minDistance = tmp;
			closestPoint = p;
		}
	}
	return closestPoint;
}

sf::Vector2f* castSingle(sf::Vector2f pos, sf::Vector2f rayDir, sf::Vertex wall[2])
{
	float x1 = pos.x;
	float y1 = pos.y;
	float x2 = x1 + rayDir.x;
	float y2 = y1 + rayDir.y;

	float x3 = wall[0].position.x;
	float y3 = wall[0].position.y;
	float x4 = wall[1].position.x;
	float y4 = wall[1].position.y;

	float den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	if (den == 0)
		return NULL;
	float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den;
	float u = ((x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2)) / den;
	if (t < 0. || u < 0. || u > 1.)
		return NULL;
	return new sf::Vector2f(x1 + t * (x2 - x1), y1 + t * (y2 - y1));
}

void setDir(playerStruct* player, sf::Vector2f dir)
{
	player->dir.x = dir.x - player->pos.x;
	player->dir.y = dir.y - player->pos.y;
}

void viewDir(sf::Vector2f* rays, float dirDeg)
{
	static const double pi = 3.14159265358979323846;
	for (int i = 0; i < FOV * ACCURACY; i++)
	{
		float rad = (1. * i / ACCURACY + dirDeg) * pi / 180.;
		rays[i] = sf::Vector2f(cos(rad), sin(rad));
	}
}

void addBox(sf::Vector2f pos, sf::Vertex walls[][2], int index, int size)
{
	walls[index][0] = sf::Vertex(pos);
	walls[index][1] = sf::Vertex(sf::Vector2f(pos.x, pos.y + size));
	walls[index + 1][0] = sf::Vertex(sf::Vector2f(pos.x, pos.y + size));
	walls[index + 1][1] = sf::Vertex(sf::Vector2f(pos.x + size, pos.y + size));
	walls[index + 2][0] = sf::Vertex(sf::Vector2f(pos.x + size, pos.y + size));
	walls[index + 2][1] = sf::Vertex(sf::Vector2f(pos.x + size, pos.y));
	walls[index + 3][0] = sf::Vertex(sf::Vector2f(pos.x + size, pos.y));
	walls[index + 3][1] = sf::Vertex(sf::Vector2f(pos.x, pos.y));
}