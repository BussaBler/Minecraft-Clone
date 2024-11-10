#pragma once
#include "VulkanIncludes.h"
#include "TupleHash.h"
#include "Generation.h"
#include <glm/glm.hpp>
#include <vector>
#include <bitset>
#include <unordered_map>
#include <thread>

struct Vertex {
	uint8_t posX;   // x component, range 0-31
	uint8_t posY;   // y component, range 0-31
	uint8_t posZ;   // z component, range 0-31
	glm::vec2 texCoord;
	char normal;
	
	Vertex(glm::vec3 pos, glm::vec2 texCoord, char normal)
		: posX(static_cast<uint8_t>(pos.x)), 
		posY(static_cast<uint8_t>(pos.y)), 
		posZ(static_cast<uint8_t>(pos.z)), 
		texCoord(texCoord),                
		normal(normal) {};
};

struct ChunkMesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

class Chunk {
public:
	Chunk(int size, glm::vec3 pos, VkDevice device);
	~Chunk();

	Chunk(const Chunk&) = delete;
	Chunk& operator=(const Chunk&) = delete;
	Chunk(Chunk&&) = default;
	Chunk& operator=(Chunk&&) = default;

	void generateChunk();
	bool isSolid(glm::vec3 pos);
	bool isSolid(glm::vec3 localPos, std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks, BLOCK_TYPE blockType) const;
	static void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int& indexOffset, glm::vec3 face[4], std::array<glm::vec2, 4>& tex, glm::vec3 pos, char dir);
	void generateChunkMesh(std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks);
	void addBlock(glm::vec3 pos, BLOCKS block, std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks);
	void removeBlock(glm::vec3 pos, std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks);

public:
	int indexOffset = 0;
	int transparentIndexOffset = 0;

	int size;
	bool dirty; // pre-delete
	bool ready; // all set for render
	bool baked; // vertices added
	bool visible;
	std::vector<BLOCKS> blocks;

	VkDevice device;

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

	VkBuffer liquidVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory liquidVertexBufferMemory = VK_NULL_HANDLE;

	VkBuffer liquidIndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory liquidIndexBufferMemory = VK_NULL_HANDLE;

	glm::vec3 pos;
	ChunkMesh mesh;
	ChunkMesh transparentMesh;

	std::thread meshThread;
};

