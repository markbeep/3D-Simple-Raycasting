#include "Platform/Platform.hpp"
#include <cmath>
#include <iostream>

const int FOV = 90;			  // how many degrees to see (Field of view)
const int ACCURACY = 3;		  // how many rays should be shot out for every degree of FOV
const float MOVESPEED = 0.09; // how fast to move around
const float TURNSPEED = 0.25; // how fast to turn around
const bool DRAWMAP = false;

static const double pi = 3.14159265358979323846;
static const double r = pi / 180.;

// the player is defined as a simple struct
struct playerStruct
{
	sf::Vector2f pos;	// the position of the player
	float dir;			// the direction the player is looking (in deg)
	sf::Vector2f* rays; // array of rays going out in a direction
};

sf::Vector2f* castSingle(sf::Vector2f pos, sf::Vector2f rayDir, sf::Vertex wall[2]);
float calcPerpDistance(sf::Vector2f a, sf::Vector2f b);
float lazyCalculateDistance(sf::Vector2f a, sf::Vector2f b);
sf::Vector2f* castAll(sf::Vector2f pos, sf::Vector2f rayDir, sf::Vertex walls[][2], int n);
void setViewDir(sf::Vector2f* rays, float dirDeg);
void addBox(sf::Vector2f pos, sf::Vertex walls[][2], int index, int size);
sf::Vector2f* degToVector(float deg);

int main()
{
	util::Platform platform;

	sf::RenderWindow window;
	float screenScalingFactor = platform.getScreenScalingFactor(window.getSystemHandle());
	window.create(sf::VideoMode(800.0f * screenScalingFactor, 800.0f * screenScalingFactor), "Raytracing Practice");
	platform.setIcon(window.getSystemHandle());
	sf::Vector2u size = window.getSize();

	sf::Event event;

	const int TOTAL_BOXES = 64;
	const int TOTAL_WALLS = 4 + TOTAL_BOXES * 4;
	// draw 4 walls surrounding the field
	sf::Vertex walls[TOTAL_WALLS][2] = {
		{ sf::Vertex(sf::Vector2f(0, 0)), sf::Vertex(sf::Vector2f(0, size.y)) },
		{ sf::Vertex(sf::Vector2f(0, size.y)), sf::Vertex(sf::Vector2f(size.x, size.y)) },
		{ sf::Vertex(sf::Vector2f(size.x, 0)), sf::Vertex(sf::Vector2f(size.x, size.y)) },
		{ sf::Vertex(sf::Vector2f(0, 0)), sf::Vertex(sf::Vector2f(size.x, 0)) }
	};
	// draw boxes
	int xBoxes = 8, yBoxes = 8;
	for (int i = 0; i < xBoxes; i++)
	{
		for (int j = 0; j < yBoxes; j++)
		{
			addBox(sf::Vector2f(i * 70 + 50, j * 70 + 50), walls, i * yBoxes * 4 + j * 4 + 4, 50);
		}
	}

	sf::Vector2f arr[FOV * ACCURACY];
	playerStruct player = { sf::Vector2f(200, 600), 0, arr };

	sf::CircleShape viewPoint(5);
	viewPoint.setOrigin(sf::Vector2f(5, 5));

	// floor vertices
	sf::VertexArray floor(sf::Quads, 4);
	floor[0].position = sf::Vector2f(0, size.y / 2);
	floor[1].position = sf::Vector2f(0, size.y);
	floor[2].position = sf::Vector2f(size.x, size.y);
	floor[3].position = sf::Vector2f(size.x, size.y / 2);
	floor[0].color = sf::Color::Black;
	floor[1].color = sf::Color::White;
	floor[2].color = sf::Color::White;
	floor[3].color = sf::Color::Black;

	// sky vertices
	sf::VertexArray sky(sf::Quads, 4);
	sky[0].position = sf::Vector2f(0, 0);
	sky[1].position = sf::Vector2f(0, size.y / 2);
	sky[2].position = sf::Vector2f(size.x, size.y / 2);
	sky[3].position = sf::Vector2f(size.x, 0);
	sky[0].color = sf::Color::Blue;
	sky[1].color = sf::Color::Black;
	sky[2].color = sf::Color::Black;
	sky[3].color = sf::Color::Blue;

	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// handle movement
		sf::Vector2f heading = *degToVector(player.dir + FOV / 2.0);	  // for going forward/backwards
		sf::Vector2f perpendicular = sf::Vector2f(heading.y, -heading.x); // for going left/right
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		{
			player.pos.x += heading.x * MOVESPEED;
			player.pos.y += heading.y * MOVESPEED;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		{
			player.pos.x -= heading.x * MOVESPEED;
			player.pos.y -= heading.y * MOVESPEED;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			player.pos.x += perpendicular.x * MOVESPEED;
			player.pos.y += perpendicular.y * MOVESPEED;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			player.pos.x -= perpendicular.x * MOVESPEED;
			player.pos.y -= perpendicular.y * MOVESPEED;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
		{
			player.dir -= TURNSPEED;
			if (player.dir < 0)
				player.dir = 360;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
		{
			player.dir += TURNSPEED;
			if (player.dir > 360)
				player.dir = 0;
		}

		setViewDir(arr, player.dir); // sets the view direction

		window.clear();

		// draws the sky and then the floor
		window.draw(sky);
		window.draw(floor);

		for (int i = 0; i < FOV * ACCURACY; i++)
		{

			sf::Vector2f* point = castAll(player.pos, player.rays[i], walls, TOTAL_WALLS);
			if (point != NULL)
			{
				float dist = calcPerpDistance(player.pos, *point);
				int height = size.y;
				int drawHeight = height / dist * 10; // screen height / dist
				int drawStart = -drawHeight / 2 + height / 2;
				if (drawStart < 0)
					drawStart = 0;
				float width = 1.0 * size.x / (FOV * ACCURACY);
				sf::RectangleShape rect(sf::Vector2f(width, drawHeight));
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

		if (DRAWMAP)
		{
			// draws the player on the map
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

/**
 * @brief Checks a single ray with all walls to find the closest intersection
 * @return sf::Vector2f* or NULL if no intersection found
 */
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

/**
 * @brief Finds the intersection of a ray and a wall
 * @return sf::Vector2f* or NULL if no intersection found
 */
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

sf::Vector2f* degToVector(float deg)
{
	float rad = deg * r;
	return new sf::Vector2f(cos(rad), sin(rad));
}

/**
 * @brief Sets the view direction of the player and updates
 * all the rays
 */
void setViewDir(sf::Vector2f* rays, float dirDeg)
{
	for (int i = 0; i < FOV * ACCURACY; i++)
	{
		float rad = (1. * i / ACCURACY + dirDeg) * r;
		rays[i] = sf::Vector2f(cos(rad), sin(rad));
	}
}

/**
 * @brief Adds a simple size*size box
 */
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