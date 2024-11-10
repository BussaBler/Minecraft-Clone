#include "Game.h"

void Game::initGame(VkDevice device) {
	for (int x = -renderDistance; x <= renderDistance; ++x) {
		for (int y = -renderDistance; y < renderDistance; ++y) {
            for (int z = -renderDistance; z < renderDistance; z++) {
                this->chunksQueue.emplace(x, y, z);
            }
		}
	}
	if (!chunksQueue.empty()) {
		while (!chunksQueue.empty()) {
			glm::vec3 pos = chunksQueue.front();
            std::tuple<int, int, int> next = {pos.x, pos.y, pos.z};
			chunksQueue.pop();

			if (worldChunks.find(next) == worldChunks.end()) {
				worldChunks.try_emplace(next, CHUNK_SIZE, pos, device);
			}
		}
	}
}

void Game::update(VkDevice device) {
	addChunks(device);
}

static Frustum extractFrustumPlanes(const glm::mat4& viewProjectionMatrix) {
    Frustum frustum;

    // Left plane
    frustum.planes[0] = glm::vec4(
        viewProjectionMatrix[0][3] + viewProjectionMatrix[0][0],
        viewProjectionMatrix[1][3] + viewProjectionMatrix[1][0],
        viewProjectionMatrix[2][3] + viewProjectionMatrix[2][0],
        viewProjectionMatrix[3][3] + viewProjectionMatrix[3][0]
    );

    // Right plane
    frustum.planes[1] = glm::vec4(
        viewProjectionMatrix[0][3] - viewProjectionMatrix[0][0],
        viewProjectionMatrix[1][3] - viewProjectionMatrix[1][0],
        viewProjectionMatrix[2][3] - viewProjectionMatrix[2][0],
        viewProjectionMatrix[3][3] - viewProjectionMatrix[3][0]
    );

    // Top plane
    frustum.planes[2] = glm::vec4(
        viewProjectionMatrix[0][3] - viewProjectionMatrix[0][1],
        viewProjectionMatrix[1][3] - viewProjectionMatrix[1][1],
        viewProjectionMatrix[2][3] - viewProjectionMatrix[2][1],
        viewProjectionMatrix[3][3] - viewProjectionMatrix[3][1]
    );

    // Bottom plane
    frustum.planes[3] = glm::vec4(
        viewProjectionMatrix[0][3] + viewProjectionMatrix[0][1],
        viewProjectionMatrix[1][3] + viewProjectionMatrix[1][1],
        viewProjectionMatrix[2][3] + viewProjectionMatrix[2][1],
        viewProjectionMatrix[3][3] + viewProjectionMatrix[3][1]
    );

    // Near plane
    frustum.planes[4] = glm::vec4(
        viewProjectionMatrix[0][3] + viewProjectionMatrix[0][2],
        viewProjectionMatrix[1][3] + viewProjectionMatrix[1][2],
        viewProjectionMatrix[2][3] + viewProjectionMatrix[2][2],
        viewProjectionMatrix[3][3] + viewProjectionMatrix[3][2]
    );

    // Far plane
    frustum.planes[5] = glm::vec4(
        viewProjectionMatrix[0][3] - viewProjectionMatrix[0][2],
        viewProjectionMatrix[1][3] - viewProjectionMatrix[1][2],
        viewProjectionMatrix[2][3] - viewProjectionMatrix[2][2],
        viewProjectionMatrix[3][3] - viewProjectionMatrix[3][2]
    );

    // Normalize the planes
    for (int i = 0; i < 6; i++) {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        frustum.planes[i] /= length;
    }

    return frustum;
}

static bool isChunkInFrustum(const Frustum& frustum, const glm::vec3& chunkMin, const glm::vec3& chunkMax) {
    for (int i = 0; i < 6; i++) {
        glm::vec3 normal = glm::vec3(frustum.planes[i]);

        // Find the positive vertex (the one furthest along the frustum normal)
        glm::vec3 positiveVertex = glm::vec3(
            (normal.x > 0.0f) ? chunkMax.x : chunkMin.x,
            (normal.y > 0.0f) ? chunkMax.y : chunkMin.y,
            (normal.z > 0.0f) ? chunkMax.z : chunkMin.z
        );

        // If the positive vertex is outside the frustum plane, cull the chunk
        if (glm::dot(positiveVertex, normal) + frustum.planes[i].w < 0.0f) {
            return false;  // Outside this plane
        }
    }

    return true; // Chunk is visible
}

void Game::addChunks(VkDevice device) {
	glm::vec3 currentChunkPos = floor(worldCamera.pos / (float)CHUNK_SIZE);

	if (currentChunkPos != lastChunkPos) {
		lastChunkPos = currentChunkPos;
        for (int x = -renderDistance; x <= renderDistance; ++x) {
            for (int y = -renderDistance; y < renderDistance; ++y) {
                for (int z = -renderDistance; z < renderDistance; z++) {
                    this->chunksQueue.emplace(currentChunkPos.x + x, currentChunkPos.y + y, currentChunkPos.z + z);
                }
            }
        }
	}
	if (!chunksQueue.empty()) {
        while (!chunksQueue.empty()) {
            glm::vec3 pos = chunksQueue.front();
            std::tuple<int, int, int> next = { pos.x, pos.y, pos.z };
            chunksQueue.pop();

            if (worldChunks.find(next) == worldChunks.end()) {
                worldChunks.try_emplace(next, CHUNK_SIZE, pos, device);
            }
        }
	}

    Frustum frustum = extractFrustumPlanes(worldCamera.proj * worldCamera.view);

	for (auto it = worldChunks.begin(); it != worldChunks.end(); ) {
		float chunkX = it->second.pos.x;
		float chunkY = it->second.pos.y;
		float chunkZ = it->second.pos.z;

        if (isChunkInFrustum(frustum, it->second.pos * (float)CHUNK_SIZE, it->second.pos * (float)CHUNK_SIZE + (float)CHUNK_SIZE)) {
            it->second.visible = true;
        }
        else {
            it->second.visible = false;
        }   

		if (abs(chunkX - currentChunkPos.x) > renderDistance || abs(chunkY - currentChunkPos.y) > renderDistance
			|| abs(chunkZ - currentChunkPos.z) > renderDistance) {
			if (it->second.dirty) {
				it = worldChunks.erase(it);
			}
			else {
				it->second.dirty = true;
				it++;
			}
		}
		else {
			it++;
		}
	}
}

// based on a DDA algorithm
HitResult Game::raycastBlock() {
    glm::vec3 rayOrigin = this->worldCamera.pos;
    glm::vec3 rayDir = glm::normalize(this->worldCamera.forwards);  

    glm::vec3 blockPos = glm::floor(rayOrigin);  
    glm::vec3 rayDelta = glm::abs(1.0f / rayDir);  

    glm::ivec3 step;  
    glm::vec3 sideDist;  

    for (int i = 0; i < 3; i++) {
        if (rayDir[i] < 0) {
            step[i] = -1;
            sideDist[i] = (rayOrigin[i] - blockPos[i]) * rayDelta[i];
        }
        else {
            step[i] = 1;
            sideDist[i] = (blockPos[i] + 1.0f - rayOrigin[i]) * rayDelta[i];
        }
    }

    float rayLength = 100.0f;
    int maxSteps = static_cast<int>(rayLength / glm::length(rayDir));
    int faceHit = -1;

    for (int i = 0; i < maxSteps; i++) {
        
        glm::vec3 currentChunkPos = floor(blockPos / (float)CHUNK_SIZE);
        std::tuple<int, int, int> currentChunkTuple = { currentChunkPos.x, currentChunkPos.y, currentChunkPos.z };

        auto chunkIt = worldChunks.find(currentChunkTuple);
        if (chunkIt != worldChunks.end()) {
            Chunk& chunk = chunkIt->second;

            glm::vec3 chunkWorldPos = currentChunkPos * static_cast<float>(CHUNK_SIZE);
            glm::vec3 localBlockPos = blockPos - chunkWorldPos;

            if (localBlockPos.x >= 0 && localBlockPos.x < CHUNK_SIZE &&
                localBlockPos.y >= 0 && localBlockPos.y < CHUNK_SIZE &&
                localBlockPos.z >= 0 && localBlockPos.z < CHUNK_SIZE) {

                int blockIndex = static_cast<int>(localBlockPos.x) +
                    static_cast<int>(localBlockPos.y) * CHUNK_SIZE +
                    static_cast<int>(localBlockPos.z) * CHUNK_SIZE * CHUNK_SIZE;

                if (blockIndex >= 0 && blockIndex < chunk.blocks.size() && chunk.blocks[blockIndex] != AIR_BLOCK) {
                    return { blockPos, faceHit, chunk.blocks[blockIndex] };
                }
            }
        }

        // traverse the grid based on the shortest distance to the next boundary (x, y, or z)
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            blockPos.x += step.x;
            sideDist.x += rayDelta.x;
            faceHit = (step.x == 1) ? 0 : 1;  // left or right face
        }
        else if (sideDist.y < sideDist.z) {
            blockPos.y += step.y;
            sideDist.y += rayDelta.y;
            faceHit = (step.y == 1) ? 2 : 3;  // back or front face
        }
        else {
            blockPos.z += step.z;
            sideDist.z += rayDelta.z;
            faceHit = (step.z == 1) ? 4 : 5;  // bottom or top face
        }
    }

    return { glm::vec3(-1), -1 };
}

