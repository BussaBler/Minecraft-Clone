#include "Generation.h"
#include <OpenSimplexNoise.hh>

static OSN::Noise<2> noise2D;
static OSN::Noise<3> noise3D;

std::array<Noise, 2> surfaceSettings = { Noise(0.01f, 20.0f, 0), Noise(0.1f, 3.0f, 0) };
std::array<Noise, 1> caveSettings = { Noise(0.05f, 1.0f, 0, 100, 0.5f) };
std::array<Noise, 1> oreSettings = { Noise(0.05f, 1.0f, 8.54f, 0, 0.75f, COAL_ORE_BLOCK) };
std::array<Noise, 1> treeSettings = { Noise(4.25f, 1.0f, 8.54f, 0, 0.75f) };

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
    int trunkHeight = 4 + (rand() % 3);  // Random trunk height between 4 and 6
    int canopyHeight = trunkHeight - 1;

    for (int i = 0; i < trunkHeight; i++) {
        if (x >= 0 && x < size && y >= 0 && y < size && z + i >= 0 && z + i < size) {
            blocks[x + y * size + (z + i) * size * size] = OAK_LOG_BLOCK;
        }
    }

    int canopyRadius = 2;  // Leaves extend 2 blocks around the trunk
    for (int dx = -canopyRadius; dx <= canopyRadius; dx++) {
        for (int dy = -canopyRadius; dy <= canopyRadius; dy++) {
            for (int dz = -canopyRadius; dz <= canopyRadius; dz++) {
                if (dx * dx + dy * dy + dz * dz <= canopyRadius * canopyRadius) {  // Circular canopy
                    int leafX = x + dx;
                    int leafY = y + dy;
                    int leafZ = z + canopyHeight + dz;

                    if (leafX >= 0 && leafX < size && leafY >= 0 && leafY < size && leafZ >= 0 && leafZ < size) {
                        if (!(dx == 0 && dy == 0)) {  // Avoid replacing the trunk with leaves
                            blocks[leafX + leafY * size + leafZ * size * size] = OAK_LEAVES_BLOCK;
                        }
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

            float noiseTreeVal = noise2D.eval(
                (float)((x + startX) * treeSettings[0].frequency) + treeSettings[0].offset,
                (float)((y + startY) * treeSettings[0].frequency) + treeSettings[0].offset
            ) * treeSettings[0].amplitude;

            for (int z = -6; z < size + 6; z++) {
                if (noiseTreeVal > treeSettings[0].probability && z + startZ == noiseZ) {
                    placeTree(x, y, z + 1, blocks, size);
                }
            }

        }
    }
}
