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
	glm::vec3 pos;         
	glm::u16vec2 texCoord;
	char normal;

	Vertex(glm::vec3 pos, glm::vec2 texCoord, char normal)
		: pos(pos), texCoord(glm::u16vec2(texCoord * 65535.0f)), normal(normal) {};
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
	bool isSolid(glm::vec3 localPos, std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks) const;
	static void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int& indexOffset, glm::vec3 face[4], std::array<glm::vec2, 4>& tex, glm::vec3 pos, char dir);
	void generateChunkMesh(std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks);

public:
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
	glm::vec3 pos;
	ChunkMesh mesh;

	std::thread meshThread;
};

