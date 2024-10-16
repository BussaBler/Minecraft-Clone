#include "Chunk.h"

glm::vec3 leftFace[4] = { {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
glm::vec3 rightFace[4] = { {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f} };
glm::vec3 backFace[4] = { {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
glm::vec3 frontFace[4] = { {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f} };
glm::vec3 bottomFace[4] = { {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f} };
glm::vec3 topFace[4] = { {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} };

Chunk::Chunk(int size, glm::vec3 pos, VkDevice device) : size(size), pos(pos), dirty(false), ready(false), visible(false), baked(false), device(device) {
    this->meshThread = std::thread(&Chunk::generateChunk, this);
}

Chunk::~Chunk() {
    if (this->meshThread.joinable()) {
        this->meshThread.join();
    }

    vkDeviceWaitIdle(device);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
}

void Chunk::generateChunk() {
    Generation::generateChunkData(this->blocks, pos, size);

    std::vector<BLOCKS> frontData, backData, leftData, rightData, upData, downData;
    glm::vec3 frontPos = pos + glm::vec3(0, 1, 0);  // Front chunk
    glm::vec3 backPos = pos + glm::vec3(0, -1, 0);  // Back chunk
    glm::vec3 leftPos = pos + glm::vec3(-1, 0, 0);  // Left chunk
    glm::vec3 rightPos = pos + glm::vec3(1, 0, 0);  // Right chunk
    glm::vec3 upPos = pos + glm::vec3(0, 0, 1);     // Upper chunk
    glm::vec3 downPos = pos + glm::vec3(0, 0, -1);  // Lower chunk

    Generation::generateChunkData(frontData, frontPos, size);
    Generation::generateChunkData(backData, backPos, size);
    Generation::generateChunkData(leftData, leftPos, size);
    Generation::generateChunkData(rightData, rightPos, size);
    Generation::generateChunkData(upData, upPos, size);
    Generation::generateChunkData(downData, downPos, size);

    this->indexOffset = 0;

    this->mesh.vertices.reserve(size * size * size * 24);
    this->mesh.indices.reserve(size * size * size * 36);

    for (int x = 0; x < size; ++x) {
        for (int y = 0; y < size; ++y) {
            for (int z = 0; z < size; ++z) {
                BLOCKS blockType = blocks[x + y * size + z * size * size];

                if (blockType == AIR_BLOCK) {
                    continue;
                }

                Block block = Block(blockType);
                glm::vec3 worldPos = glm::vec3(x, y, z); // +this->pos * static_cast<float>(this->size);

                // Left face (x - 1)
                if (x == 0) {
                    if (Block(leftData[size - 1 + y * size + z * size * size]).transparent) {  // Get from the right edge of the left chunk
                        addFace(mesh.vertices, mesh.indices, indexOffset, leftFace, block.left, worldPos, 0);
                    }
                }
                else if (Block(blocks[(x - 1) + y * size + z * size * size]).transparent) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, leftFace, block.left, worldPos, 0);
                }

                // Right face (x + 1)
                if (x == this->size - 1) {
                    if (Block(rightData[0 + y * size + z * size * size]).transparent) {  // Get from the left edge of the right chunk
                        addFace(mesh.vertices, mesh.indices, indexOffset, rightFace, block.right, worldPos, 1);
                    }
                }
                else if (Block(blocks[(x + 1) + y * size + z * size * size]).transparent) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, rightFace, block.right, worldPos, 1);
                }

                // Back face (y - 1)
                if (y == 0) {
                    if (Block(backData[x + (size - 1) * size + z * size * size]).transparent) {  // Get from the front edge of the back chunk
                        addFace(mesh.vertices, mesh.indices, indexOffset, backFace, block.back, worldPos, 2);
                    }
                }
                else if (Block(blocks[x + (y - 1) * size + z * size * size]).transparent) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, backFace, block.back, worldPos, 2);
                }

                // Front face (y + 1)
                if (y == this->size - 1) {
                    if (Block(frontData[x + 0 * size + z * size * size]).transparent) {  // Get from the back edge of the front chunk
                        addFace(mesh.vertices, mesh.indices, indexOffset, frontFace, block.front, worldPos, 3);
                    }
                }
                else if (Block(blocks[x + (y + 1) * size + z * size * size]).transparent) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, frontFace, block.front, worldPos, 3);
                }

                // Bottom face (z - 1)
                if (z == 0) {
                    if (Block(downData[x + y * size + (size - 1) * size * size]).transparent) {  // Get from the top edge of the bottom chunk
                        addFace(mesh.vertices, mesh.indices, indexOffset, bottomFace, block.bottom, worldPos, 4);
                    }
                }
                else if (Block(blocks[x + y * size + (z - 1) * size * size]).transparent) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, bottomFace, block.bottom, worldPos, 4);
                }

                // Top face (z + 1)
                if (z == this->size - 1) {
                    if (Block(upData[x + y * size + 0 * size * size]).transparent) {  // Get from the bottom edge of the top chunk
                        addFace(mesh.vertices, mesh.indices, indexOffset, topFace, block.top, worldPos, 5);
                    }
                }
                else if (Block(blocks[x + y * size + (z + 1) * size * size]).transparent) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, topFace, block.top, worldPos, 5);
                }
            }
        }
    }

    this->baked = true;
}

void Chunk::addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int& indexOffset, glm::vec3 face[4], std::array<glm::vec2, 4>& tex, glm::vec3 pos, char dir) {
	for (int i = 0; i < 4; i++) {
		vertices.emplace_back( face[i] + pos, tex[i], dir );
	}

	indices.emplace_back(indexOffset + 0);
	indices.emplace_back(indexOffset + 3);
	indices.emplace_back(indexOffset + 2);
	indices.emplace_back(indexOffset + 2);
	indices.emplace_back(indexOffset + 1);
	indices.emplace_back(indexOffset + 0);

	indexOffset += 4;
}

void Chunk::generateChunkMesh(std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks) {
    this->mesh.vertices.clear();
    this->mesh.indices.clear();

    this->indexOffset = 0;

    for (int x = 0; x < size; ++x) {
        for (int y = 0; y < size; ++y) {
            for (int z = 0; z < size; ++z) {

                BLOCKS blockType = blocks[x + y * size + z * size * size];

                if (blockType == AIR_BLOCK) {
                    continue;
                }

                Block block = Block(blockType);
                glm::vec3 worldPos = glm::vec3(x, y, z);

                if (!isSolid({ x - 1, y, z }, worldChunks)) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, leftFace, block.left, worldPos, 0);
                }
                if (!isSolid({ x + 1, y, z }, worldChunks)) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, rightFace, block.right, worldPos, 1);
                }
                if (!isSolid({ x, y - 1, z }, worldChunks)) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, backFace, block.back, worldPos, 2);
                }
                if (!isSolid({ x, y + 1, z }, worldChunks)) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, frontFace, block.front, worldPos, 3);
                }
                if (!isSolid({ x, y, z - 1 }, worldChunks)) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, bottomFace, block.bottom, worldPos, 4);
                }
                if (!isSolid({ x, y, z + 1 }, worldChunks)) {
                    addFace(mesh.vertices, mesh.indices, indexOffset, topFace, block.top, worldPos, 5);
                }
            }
        }
    }

    this->baked = true;
}

void Chunk::addBlock(glm::vec3 pos, BLOCKS blockType, std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks) {
    int posX = (int)pos.x;
    int posY = (int)pos.y;
    int posZ = (int)pos.z;

    int blockIndex = posX + posY * size + posZ * size * size;

    blocks[blockIndex] = blockType;

    Block block = Block(blockType);

    glm::vec3 worldPos = glm::vec3(posX, posY, posZ);

    if (!isSolid({ posX - 1, posY, posZ }, worldChunks)) {
        addFace(mesh.vertices, mesh.indices, indexOffset, leftFace, block.left, worldPos, 0);
    }
    if (!isSolid({ posX + 1, posY, posZ }, worldChunks)) {
        addFace(mesh.vertices, mesh.indices, indexOffset, rightFace, block.right, worldPos, 1);
    }
    if (!isSolid({ posX, posY - 1, posZ }, worldChunks)) {
        addFace(mesh.vertices, mesh.indices, indexOffset, backFace, block.back, worldPos, 2);
    }
    if (!isSolid({ posX, posY + 1, posZ }, worldChunks)) {
        addFace(mesh.vertices, mesh.indices, indexOffset, frontFace, block.front, worldPos, 3);
    }
    if (!isSolid({ posX, posY, posZ - 1 }, worldChunks)) {
        addFace(mesh.vertices, mesh.indices, indexOffset, bottomFace, block.bottom, worldPos, 4);
    }
    if (!isSolid({ posX, posY, posZ + 1 }, worldChunks)) {
        addFace(mesh.vertices, mesh.indices, indexOffset, topFace, block.top, worldPos, 5);
    }

    this->ready = false;

}

void Chunk::removeBlock(glm::vec3 pos, std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks) {
    int posX = static_cast<int>(pos.x);
    int posY = static_cast<int>(pos.y);
    int posZ = static_cast<int>(pos.z);

    int blockIndex = posX + posY * size + posZ * size * size;

    blocks[blockIndex] = AIR_BLOCK;

    this->baked = false;

    generateChunkMesh(worldChunks);

    this->ready = false;
}

bool Chunk::isSolid(glm::vec3 pos) {
	return this->blocks[pos.x + this->size * pos.y + this->size * this->size * pos.z] != AIR_BLOCK;
}

bool Chunk::isSolid(glm::vec3 localPos, std::unordered_map<std::tuple<int, int, int>, Chunk>& worldChunks) const {
    std::tuple<int, int, int> neighborPos = std::make_tuple((int)this->pos.x, (int)this->pos.y, (int)this->pos.z);

    if (localPos.x < 0) {
        std::get<0>(neighborPos) -= 1;
        localPos.x = size - 1;
    }
    else if (localPos.x >= size) {
        std::get<0>(neighborPos) += 1;
        localPos.x = 0;
    }

    if (localPos.y < 0) {
        std::get<1>(neighborPos) -= 1;
        localPos.y = size - 1;
    }
    else if (localPos.y >= size) {
        std::get<1>(neighborPos) += 1;
        localPos.y = 0;
    }

    if (localPos.z < 0) {
        std::get<2>(neighborPos) -= 1;
        localPos.z = size - 1;
    }
    else if (localPos.z >= size) {
        std::get<2>(neighborPos) += 1;
        localPos.z = 0;
    }

    auto neighborIt = worldChunks.find(neighborPos);
    if (neighborIt != worldChunks.end()) {
        return !Block(neighborIt->second.blocks[(int)localPos.x + (int)localPos.y * size + (int)localPos.z * size * size]).transparent;
    }

    return false;
}
