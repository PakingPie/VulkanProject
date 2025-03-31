#include "lve_app.hpp"

// std library
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
	lve::LveApp lveApp{};

	try {
		lveApp.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}