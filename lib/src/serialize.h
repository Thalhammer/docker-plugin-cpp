#pragma once
#include <string>

namespace docker_plugin {
	template <typename T>
	std::string to_json(const T& e);
	template <typename T>
	T from_json(const std::string& str);
} // namespace docker_plugin