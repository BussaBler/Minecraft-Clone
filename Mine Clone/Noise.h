#pragma once
#include "Block.h"
#include <glm/glm.hpp>
#include <array>

class Noise { 
public:
	Noise(float frequency, float amplitude, float offset, int maxHeight = 0, float probability = 0.0f, BLOCKS block = DIRT_BLOCK);
public:
	float amplitude;
	float frequency;
	float offset;

	float probability;
	int maxHeight;
	BLOCKS block;
};

