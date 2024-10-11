#include "Block.h"


Block::Block(BLOCKS type) {
	switch (type)
	{
	case GRASS_BLOCK:
		this->left = this->back = { glm::vec2(0.0f, 0.0f), {0.05f, 0.0f}, {0.05f, 0.05f}, {0.0f, 0.05f} }; //left-front
		this->right = this->front = { glm::vec2(0.05f, 0.0f), {0.05f, 0.05f}, {0.0f, 0.05f}, {0.0f, 0.0f} }; //right-back
		this->top = { glm::vec2(0.05f, 0.0f), {0.10f, 0.0f}, {0.10f, 0.05f}, {0.05f, 0.05f} }; //top
		this->bottom = { glm::vec2(0.10f, 0.0f), {0.15f, 0.0f}, {0.15f, 0.05f}, {0.10f, 0.05f} }; // bottom
		this->transparent = false;
		break;
	case DIRT_BLOCK:
		this->left = this->back = { glm::vec2(0.10f, 0.0f), {0.15f, 0.0f}, {0.15f, 0.05f}, {0.10f, 0.05f} };
		this->right = this->front = { glm::vec2(0.15f, 0.0f), {0.15f, 0.05f}, {0.10f, 0.05f}, {0.10f, 0.0f} };
		this->top = this->bottom = { glm::vec2(0.10f, 0.0f), {0.15f, 0.0f}, {0.15f, 0.05f}, {0.10f, 0.05f} };
		this->transparent = false;
		break;
	case OAK_LEAVES_BLOCK:
		this->left = this->back = { glm::vec2(0.20f, 0.0f), {0.25f, 0.0f}, {0.25f, 0.05f}, {0.20f, 0.05f} };
		this->right = this->front = { glm::vec2(0.25f, 0.0f), {0.25f, 0.05f}, {0.20f, 0.05f}, {0.20f, 0.0f} };
		this->top = this->bottom = { glm::vec2(0.20f, 0.0f), {0.25f, 0.0f}, {0.25f, 0.05f}, {0.20f, 0.05f} };
		this->transparent = true;
		break;
	case OAK_LOG_BLOCK:
		this->left = this->back = { glm::vec2(0.25f, 0.0f), {0.30f, 0.0f}, {0.30f, 0.05f}, {0.25f, 0.05f} };
		this->right = this->front = { glm::vec2(0.30f, 0.0f), {0.30f, 0.05f}, {0.25f, 0.05f}, {0.25f, 0.0f} }; 
		this->top = this->bottom = { glm::vec2(0.30f, 0.0f), {0.35f, 0.0f}, {0.35f, 0.05f}, {0.30f, 0.05f} };
		this->transparent = false;
		break;
	case STONE_BLOCK:
		this->left = this->back = { glm::vec2(0.15f, 0.0f), {0.20f, 0.0f}, {0.20f, 0.05f}, {0.15f, 0.05f} };
		this->right = this->front = { glm::vec2(0.20f, 0.0f), {0.20f, 0.05f}, {0.15f, 0.05f}, {0.15f, 0.0f} };
		this->top = this->bottom = { glm::vec2(0.15f, 0.0f), {0.20f, 0.0f}, {0.20f, 0.05f}, {0.15f, 0.05f} };
		this->transparent = false;
		break;
	case COAL_ORE_BLOCK:
		this->left = this->back = { glm::vec2(0.0f, 0.95f), {0.05f, 0.95f}, {0.05f, 1.0f}, {0.0f, 1.0f} };
		this->right = this->front = { glm::vec2(0.05f, 0.95f), {0.05f, 1.0f}, {0.00f, 1.0f}, {0.0f, 0.95f} };
		this->top = this->bottom = { glm::vec2(0.0f, 0.95f), {0.05f, 0.95f}, {0.05f, 1.0f}, {0.0f, 1.0f} };
		this->transparent = false;
		break;
	default:
		this->left = this->front = this->right = this->back = this->top = this->bottom = {};
		this->transparent = true;
		break;
	}
}