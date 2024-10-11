#pragma once
#include <glm/glm.hpp>
#include <array>

enum BLOCKS {
	GRASS_BLOCK,
	DIRT_BLOCK,
	OAK_LEAVES_BLOCK,
	OAK_LOG_BLOCK,
	STONE_BLOCK,
	COAL_ORE_BLOCK,
	IRON_ORE_BLOCK,
	AIR_BLOCK
};

class Block {
public:
	std::array<glm::vec2, 4> left;
	std::array<glm::vec2, 4> right;
	std::array<glm::vec2, 4> front;
	std::array<glm::vec2, 4> back;
	std::array<glm::vec2, 4> top;
	std::array<glm::vec2, 4> bottom;
	bool transparent;
public:
	Block() = default;
	Block(BLOCKS type);
};

