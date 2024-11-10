#pragma once
#include <glm/glm.hpp>
#include <array>

enum BLOCKS {
	GRASS_BLOCK,
	DIRT_BLOCK,
	OAK_LEAVES_BLOCK,
	OAK_LOG_BLOCK,
	STONE_BLOCK,
	SAND_BLOCK,
	WATER_BLOCK,
	COAL_ORE_BLOCK,
	IRON_ORE_BLOCK,
	AIR_BLOCK
};

enum BLOCK_TYPE {
	SOLID,
	TRANSPARENT,
	LIQUID
};



class Block {
public:
	std::array<glm::vec2, 4> left;
	std::array<glm::vec2, 4> right;
	std::array<glm::vec2, 4> front;
	std::array<glm::vec2, 4> back;
	std::array<glm::vec2, 4> top;
	std::array<glm::vec2, 4> bottom;
	BLOCK_TYPE type;
public:
	Block() = default;
	Block(BLOCKS type);
	static void setBlockTextures(Block* pBlock, uint32_t top, uint32_t bottom, uint32_t left, uint32_t right, uint32_t front, uint32_t back);
private:
	static const std::pair<int, int> altasSize;
};
