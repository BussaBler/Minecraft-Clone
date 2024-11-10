#include "Block.h"
const std::pair<int, int> Block::altasSize(1024, 1024);

Block::Block(BLOCKS type) {
	switch (type)
	{
	case GRASS_BLOCK:
        Block::setBlockTextures(this, 1, 2, 0, 0, 0, 0);
		this->type = SOLID;
		break;
	case DIRT_BLOCK:
        Block::setBlockTextures(this, 2, 2, 2, 2, 2, 2);
		this->type = SOLID;
		break;
	case OAK_LEAVES_BLOCK:
        Block::setBlockTextures(this, 4, 4, 4, 4, 4, 4);
		this->type = TRANSPARENT;
		break;
	case OAK_LOG_BLOCK:
        Block::setBlockTextures(this, 6, 6, 5, 5, 5, 5);
		this->type = SOLID;
		break;
	case STONE_BLOCK:
        Block::setBlockTextures(this, 3, 3, 3, 3, 3, 3);
		this->type = SOLID;
		break;
	case SAND_BLOCK:
        Block::setBlockTextures(this, 7, 7, 7, 7, 7, 7);
		this->type = SOLID;
		break;
	case WATER_BLOCK:
        Block::setBlockTextures(this, 8, 8, 8, 8, 8, 8);
		this->type = LIQUID;
		break;
	case COAL_ORE_BLOCK:
        Block::setBlockTextures(this, 40, 40, 40, 40, 40, 40);
		this->type = SOLID;
		break;
	default:
		this->left = this->front = this->right = this->back = this->top = this->bottom = {};
		this->type = TRANSPARENT;
		break;
	}
}

// The ints used are relative to the face texture in the block atlas
void Block::setBlockTextures(Block* pBlock, uint32_t top, uint32_t bottom, uint32_t left, uint32_t right, uint32_t front, uint32_t back) {
    float stepX = 16.0f / Block::altasSize.first;
    float stepY = 16.0f / Block::altasSize.second;

    int blocksPerRow = Block::altasSize.first / 16;

    int leftCol = left % blocksPerRow;
    int leftRow = left / blocksPerRow;
    pBlock->left = {
        glm::vec2(leftCol * stepX, leftRow * stepY),                          
        glm::vec2((leftCol + 1) * stepX, leftRow * stepY),                    
        glm::vec2((leftCol + 1) * stepX, (leftRow + 1) * stepY),              
        glm::vec2(leftCol * stepX, (leftRow + 1) * stepY)                     
    };

    int backCol = back % blocksPerRow;
    int backRow = back / blocksPerRow;
    pBlock->back = {
        glm::vec2(backCol * stepX, backRow * stepY),                          
        glm::vec2((backCol + 1) * stepX, backRow * stepY),                    
        glm::vec2((backCol + 1) * stepX, (backRow + 1) * stepY),              
        glm::vec2(backCol * stepX, (backRow + 1) * stepY)                     
    };

    int rightCol = right % blocksPerRow;
    int rightRow = right / blocksPerRow;
    pBlock->right = {
        glm::vec2((rightCol + 1) * stepX, rightRow * stepY),                  
        glm::vec2((rightCol + 1) * stepX, (rightRow + 1) * stepY),            
        glm::vec2(rightCol * stepX, (rightRow + 1) * stepY),                  
        glm::vec2(rightCol * stepX, rightRow * stepY)                         
    };

    int frontCol = front % blocksPerRow;
    int frontRow = front / blocksPerRow;
    pBlock->front = {
        glm::vec2((frontCol + 1) * stepX, frontRow * stepY),                  
        glm::vec2((frontCol + 1) * stepX, (frontRow + 1) * stepY),            
        glm::vec2(frontCol * stepX, (frontRow + 1) * stepY),                  
        glm::vec2(frontCol * stepX, frontRow * stepY)                         
    };

    int topCol = top % blocksPerRow;
    int topRow = top / blocksPerRow;
    pBlock->top = {
        glm::vec2(topCol * stepX, topRow * stepY),                            
        glm::vec2((topCol + 1) * stepX, topRow * stepY),                      
        glm::vec2((topCol + 1) * stepX, (topRow + 1) * stepY),                
        glm::vec2(topCol * stepX, (topRow + 1) * stepY)                       
    };

    int bottomCol = bottom % blocksPerRow;
    int bottomRow = bottom / blocksPerRow;
    pBlock->bottom = {
        glm::vec2(bottomCol * stepX, bottomRow * stepY),                      
        glm::vec2((bottomCol + 1) * stepX, bottomRow * stepY),                
        glm::vec2((bottomCol + 1) * stepX, (bottomRow + 1) * stepY),          
        glm::vec2(bottomCol * stepX, (bottomRow + 1) * stepY)                 
    };
}
