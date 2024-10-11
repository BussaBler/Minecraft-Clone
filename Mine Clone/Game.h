#pragma once
#include "Chunk.h"
#include <glm/glm.hpp>
#include <queue>
#include <unordered_map>
const int CHUNK_SIZE = 32;

struct Camera {
    glm::vec3 pos;
    glm::vec3 forwards = glm::vec3(1, 0, 0);
    glm::vec3 right;
    glm::vec3 up = glm::vec3(0, 0, 1);
    glm::vec3 frames = glm::vec4(0);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 proj = glm::mat4(1);
};

struct Frustum {
    glm::vec4 planes[6]; // Left, Right, Top, Bottom, Near, Far
};

class Game {
public:
    void update(VkDevice device);
    void addChunks(VkDevice device);
    void initGame(VkDevice device);
public:
    Camera worldCamera;
    int renderDistance = 3;
    glm::vec3 lastChunkPos;
    float lastFrame = 0;
    float deltaTime = 0;
    std::unordered_map<std::tuple<int, int, int>, Chunk> worldChunks;
    std::queue<glm::vec3> chunksQueue;
};

