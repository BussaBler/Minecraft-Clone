#include "Game.h"

void Game::initGame(VkDevice device) {
	for (int x = -renderDistance; x <= renderDistance; ++x) {
		for (int y = -renderDistance; y < renderDistance; ++y) {
            for (int z = -1; z < 1; z++) {
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
                for (int z = -1; z < 1; z++) {
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