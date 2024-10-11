#include "VkContext.h"

int main() {
	VkContext context{};
	Game game{};
	VkContext::initWindow(&context, &game);
	VkContext::initVulkan(&context, &game);
	game.initGame(context.device);

	while (!glfwWindowShouldClose(context.window)) {
		glfwPollEvents();
		float currentFrame = static_cast<float>(glfwGetTime());
		game.deltaTime = currentFrame - game.lastFrame;
		game.lastFrame = currentFrame;
		game.update(context.device);
		VkContext::drawFrame(&context, &game);
		VkContext::processInput(context.window, &game);
	}

	vkDeviceWaitIdle(context.device);

	game.worldChunks.clear();
	VkContext::cleanup(&context, &game);

	return EXIT_SUCCESS;
}