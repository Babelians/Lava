#include "vulkanBase.h"

const int WindowWidth = 600, WindowHeight = 480;
const char* AppTitle = "âNÇÃçèâˆäÔÊù";

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, 0);
	auto window = glfwCreateWindow(WindowWidth, WindowHeight, AppTitle, nullptr, nullptr);

	VulkanBase vkBase;
	vkBase.initialize(window, AppTitle);
	while (glfwWindowShouldClose(window) == GLFW_FALSE)
	{
		glfwPollEvents();
	}
	glfwTerminate();
	vkBase.terminate();
	return 0;
}