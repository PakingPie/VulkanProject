#include "lve_window.hpp"

namespace lve {
	LveWindow::LveWindow(int w, int h, std::string t) : width(w), height(h), title(t){
		initWindow();
	};

	LveWindow::~LveWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void LveWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void LveWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto lveWindow = reinterpret_cast<LveWindow*>(glfwGetWindowUserPointer(window));
		lveWindow->framebufferResized = true;
		lveWindow->width = width;
		lveWindow->height = height;

	}

	void LveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}
}