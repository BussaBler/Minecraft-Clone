#include "Generation.h"
#include <OpenSimplexNoise.hh>

static OSN::Noise<2> noise2D;
static OSN::Noise<3> noise3D;

std::array<Noise, 2> surfaceSettings = { Noise(0.01f, 20.0f, 0), Noise(0.05f, 3.0f, 0) };
std::array<Noise, 1> caveSettings = { Noise(0.05f, 1.0f, 0, 100, 0.5f) };
std::array<Noise, 1> oreSettings = { Noise(0.05f, 1.0f, 8.54f, 0, 0.75f, COAL_ORE_BLOCK) };
std::array<Noise, 1> treeSettings = { Noise(4.25f, 1.0f, 8.54f, 0, 0.75f) };
const int waterLevel = 10;

void Generation::generateChunkData(std::vector<BLOCKS>& blocks, glm::vec3& pos, int size) {
    blocks.resize(size * size * size, AIR_BLOCK);

    int startX = pos.x * size;
    int startY = pos.y * size;
    int startZ = pos.z * size;

    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++) {

            int noiseZ = 20;
                
            for (int i = 0; i < surfaceSettings.size(); i++) {
                noiseZ += noise2D.eval(
                    (float)(x + startX) * surfaceSettings[i].frequency,
                    (float)(y + startY) * surfaceSettings[i].frequency) * surfaceSettings[i].amplitude;
            }

            for (int z = 0; z < size; z++) {
                int index = x + y * size + z * size * size;

                bool cave = false;

                for (int i = 0; i < caveSettings.size(); i++) {
                    if (z + startZ > caveSettings[i].maxHeight)
                        continue;
                        
                    float caveNoiseVal = noise3D.eval(
                        (float)((x + startX) * caveSettings[i].frequency) + caveSettings[i].offset,
                        (float)((y + startY) * caveSettings[i].frequency) + caveSettings[i].offset,
                        (float)((z + startZ) * caveSettings[i].frequency) + caveSettings[i].offset
                    ) * caveSettings[i].amplitude;

                    if (caveNoiseVal > caveSettings[i].probability) {
                        cave = true;
                        break;
                    }

                }

                if (z + startZ > noiseZ) {
                    if (z + startZ <= waterLevel)
                        blocks[index] = WATER_BLOCK;
                    else 
                        blocks[index] = AIR_BLOCK; 
                }
                else if (cave) {
                    blocks[index] = AIR_BLOCK; 
                }
                else {
                    bool ore = false;
                    for (int i = 0; i < oreSettings.size(); i++) {
                        if (z + startZ > oreSettings[i].maxHeight)
                            continue;
                        float noiseOreVal = noise3D.eval(
                            (float)((x + startX) * oreSettings[i].frequency) + oreSettings[i].offset,
                            (float)((y + startY) * oreSettings[i].frequency) + oreSettings[i].offset,
                            (float)((z + startZ) * oreSettings[i].frequency) + oreSettings[i].offset)
                            * oreSettings[i].amplitude;

                        if (noiseOreVal > oreSettings[i].probability) {
                            blocks[index] = oreSettings[i].block;
                            ore = true;
                            break;
                        }
                    }

                    if (!ore) {
                        if (z + startZ == noiseZ) {
                            blocks[index] = GRASS_BLOCK;
                        }
                        else if (z + startZ > 10) {
                            blocks[index] = DIRT_BLOCK;
                        }
                        else {
                            blocks[index] = STONE_BLOCK;
                        }
                    }
                }
            }
        }
    }

    Generation::generateTrees(blocks, pos, size);
}

void static placeTree(int x, int y, int z, std::vector<BLOCKS>& blocks, int size) {
    int seed = x * 73856093 ^ y * 19349663 ^ z * 83492791;
    std::srand(seed);

    int trunkHeight = 4 + 1; // (std::rand() % 3);
    int canopyStartHeight = z + trunkHeight - 1;

    for (int i = 0; i < trunkHeight; i++) {
        if (x >= 0 && x < size && y >= 0 && y < size && z + i >= 0 && z + i < size) {
            blocks[x + y * size + (z + i) * size * size] = OAK_LOG_BLOCK;
        }
    }

    if (z + trunkHeight + 1 >= 0 && z + trunkHeight + 1 < size) {
        if (x >= 0 && x < size && y >= 0 && y < size)
            blocks[x + y * size + (z + trunkHeight + 1) * size * size] = OAK_LEAVES_BLOCK;
        if (x + 1 >= 0 && x + 1 < size && y >= 0 && y < size)
            blocks[(x + 1) + y * size + (z + trunkHeight + 1) * size * size] = OAK_LEAVES_BLOCK;
        if (x - 1 >= 0 && x - 1 < size && y >= 0 && y < size)
            blocks[(x - 1) + y * size + (z + trunkHeight + 1) * size * size] = OAK_LEAVES_BLOCK;
        if (x >= 0 && x < size && y + 1 >= 0 && y + 1 < size)
            blocks[x + (y + 1) * size + (z + trunkHeight + 1) * size * size] = OAK_LEAVES_BLOCK;
        if (x >= 0 && x < size && y - 1 >= 0 && y - 1 < size)
            blocks[x + (y - 1) * size + (z + trunkHeight + 1) * size * size] = OAK_LEAVES_BLOCK;
    }

    int diagonalLeaves = 1 + (std::rand() % 3);
    if (z + trunkHeight >= 0 && z + trunkHeight < size) {
        if (x >= 0 && x < size && y >= 0 && y < size)
            blocks[x + y * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
        if (x + 1 >= 0 && x + 1 < size && y >= 0 && y < size)
            blocks[(x + 1) + y * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
        if (x - 1 >= 0 && x - 1 < size && y >= 0 && y < size)
            blocks[(x - 1) + y * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
        if (x >= 0 && x < size && y + 1 >= 0 && y + 1 < size)
            blocks[x + (y + 1) * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
        if (x >= 0 && x < size && y - 1 >= 0 && y - 1 < size)
            blocks[x + (y - 1) * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
        if (diagonalLeaves >= 1 && x + 1 >= 0 && x + 1 < size && y + 1 >= 0 && y + 1 < size)
            blocks[(x + 1) + (y + 1) * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
        if (diagonalLeaves >= 2 && x - 1 >= 0 && x - 1 < size && y - 1 >= 0 && y - 1 < size)
            blocks[(x - 1) + (y - 1) * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
        if (diagonalLeaves >= 3 && x + 1 >= 0 && x + 1 < size && y - 1 >= 0 && y - 1 < size)
            blocks[(x + 1) + (y - 1) * size + (z + trunkHeight) * size * size] = OAK_LEAVES_BLOCK;
    }

    for (int dz = 0; dz < 2; dz++) {
        int currentHeight = z + trunkHeight - 1 - dz;
        if (currentHeight >= 0 && currentHeight < size) {
            for (int dx = -2; dx <= 2; dx++) {
                for (int dy = -2; dy <= 2; dy++) {
                    if (abs(dx) + abs(dy) <= 2) {
                        int leafX = x + dx;
                        int leafY = y + dy;
                        if (leafX >= 0 && leafX < size && leafY >= 0 && leafY < size)
                            blocks[leafX + leafY * size + currentHeight * size * size] = OAK_LEAVES_BLOCK;
                    }
                }
            }
        }
    }
}

void Generation::generateTrees(std::vector<BLOCKS>& blocks, glm::vec3& pos, int size) {
    int startX = pos.x * size;
    int startY = pos.y * size;
    int startZ = pos.z * size;

    for (int x = -3; x < size + 3; x++) {
        for (int y = -3; y < size + 3; y++) {

            int noiseZ = 20;
            for (int i = 0; i < surfaceSettings.size(); i++) {
                noiseZ += noise2D.eval(
                    (float)(x + startX) * surfaceSettings[i].frequency,
                    (float)(y + startY) * surfaceSettings[i].frequency) * surfaceSettings[i].amplitude;
            }

            if (noiseZ < waterLevel)
                continue;

            float noiseTreeVal = noise2D.eval(
                (float)((x + startX) * treeSettings[0].frequency) + treeSettings[0].offset,
                (float)((y + startY) * treeSettings[0].frequency) + treeSettings[0].offset
            ) * treeSettings[0].amplitude;

            for (int z = -6; z < size + 6; z++) {
                bool cave = false;
                for (int i = 0; i < caveSettings.size(); i++) {
                    if (z + startZ > caveSettings[i].maxHeight)
                        continue;

                    float caveNoiseVal = noise3D.eval(
                        (float)((x + startX) * caveSettings[i].frequency) + caveSettings[i].offset,
                        (float)((y + startY) * caveSettings[i].frequency) + caveSettings[i].offset,
                        (float)((z + startZ) * caveSettings[i].frequency) + caveSettings[i].offset
                    ) * caveSettings[i].amplitude;

                    if (caveNoiseVal > caveSettings[i].probability) {
                        cave = true;
                        break;
                    }

                }

                if (noiseTreeVal > treeSettings[0].probability && z + startZ == noiseZ && !cave) {
                    placeTree(x, y, z + 1, blocks, size);
                }
            }

        }
    }
}
