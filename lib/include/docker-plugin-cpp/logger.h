#pragma once
#include <chrono>
#include <memory>
#include <string>

namespace docker_plugin {

	enum class log_level : int {
		error = 0,
		warning = 1,
		info = 2,
		debug = 3,
		trace = 4
	};

	/**
	 * \brief Interface to plug a logger into the framework
	 */
	class logger {
	public:
		using level = log_level;
		virtual ~logger() = default;
		virtual bool should_log(level l) const noexcept = 0;
		virtual void log(level l, const std::string& msg) const noexcept = 0;
	};

	/**
	 * \brief Very basic logger that just uses printf to log to stdout.
	 */
	struct stdout_logger : logger {
		level min_level{level::info};
		bool should_log(level l) const noexcept override;
		void log(level l, const std::string& msg) const noexcept override;
	};
} // namespace docker_plugin