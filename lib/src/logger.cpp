#include <cstdio>
#include <docker-plugin-cpp/logger.h>

namespace docker_plugin {

	bool stdout_logger::should_log(level l) const noexcept {
		return l <= min_level;
	}

	void stdout_logger::log(level l, const std::string& msg) const noexcept {
		if (!should_log(l)) return;
		static constexpr const char* const levels[] = {"error", "warning", "info", "debug", "trace"};
		auto timer = time(NULL);
		struct tm tm_info;
		localtime_r(&timer, &tm_info);
		char buffer[64];
		strftime(buffer, 64, "%c", &tm_info);
		printf("%s %s %s\n", buffer, levels[static_cast<int>(l)], msg.c_str());
	}

} // namespace docker_plugin