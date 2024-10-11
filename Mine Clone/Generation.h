#pragma once
#include "Block.h"
#include "Noise.h"
#include <vector>
#include <array>

class Generation {
public:
	static void generateChunkData(std::vector<BLOCKS>& blocks, glm::vec3& pos, int size);
private:
	static void generateTrees(std::vector<BLOCKS>& blocks, glm::vec3& pos, int size);
};

