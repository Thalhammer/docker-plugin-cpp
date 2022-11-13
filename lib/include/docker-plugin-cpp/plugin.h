#pragma once
#include <chrono>
#include <memory>
#include <set>
#include <string>

namespace docker_plugin {
	// Forward declarations
	namespace volume {
		struct driver;
	}
	namespace network {
		struct driver;
	}
	namespace ipam {
		struct driver;
	}
	class uds_server;
	class plugin_http_connection;
	class logger;

	/**
	 * \brief Main plugin class
	 *
	 * This creates a uds socket in dockers plugin folder, hosts a http 1.1 server on it and handles serialization and request handling.
	 */
	class plugin {
		plugin(const plugin&) = delete;
		plugin(plugin&&) = delete;
		plugin& operator=(const plugin&) = delete;
		plugin& operator=(plugin&&) = delete;

		logger* m_logger;
		std::unique_ptr<uds_server> m_server;
		volume::driver* m_volume_driver;
		network::driver* m_network_driver;
		ipam::driver* m_ipam_driver;

		friend class plugin_http_connection;

	public:
		/**
		 * \brief Create a new plugin with the specified name
		 * \param driver_name The name of the plugin as to be used by docker.
		 * \param log Logger implementation to use or nullptr for no logging.
		 */
		plugin(const std::string& driver_name, logger* log = nullptr);
		~plugin();

		/**
		 * \brief Register a volume driver for this plugin.
		 * \param drv Reference to the driver implementation. Needs to stay valid as long as run() is active.
		 * This causes the plugin to announce support for volume handling in
		 * Plugin.Activate and forward all plugin related calls to the handler.
		 */
		void register_volume(volume::driver& drv) noexcept { m_volume_driver = &drv; }

		/**
		 * \brief Register a network driver for this plugin.
		 * \param drv Reference to the network implementation. Needs to stay valid as long as run() is active.
		 * This causes the plugin to announce support for network handling in
		 * Plugin.Activate and forward all plugin related calls to the handler.
		 */
		void register_network(network::driver& drv) noexcept { m_network_driver = &drv; }

		/**
		 * \brief Register a ipam driver for this plugin.
		 * \param drv Reference to the ipam implementation. Needs to stay valid as long as run() is active.
		 * This causes the plugin to announce support for ipam handling in
		 * Plugin.Activate and forward all plugin related calls to the handler.
		 */
		void register_ipam(ipam::driver& drv) noexcept { m_ipam_driver = &drv; }

		/**
		 * \brief Run the mainloop with the specified timeout.
		 * \param timeout Maximum time to wait for events
		 * This needs to be called in a loop, stopping to call it
		 * will cause all I/O to halt. Handlercallbacks will be called
		 * from within this function.
		 * \return Returns 0 on success or the errno if an error occurred.
		 */
		int run(std::chrono::milliseconds timeout = std::chrono::milliseconds{1000});

		/**
		 * \brief Get the logger used by this plugin
		 */
		const logger* get_logger() const noexcept { return m_logger; }
	};

	// ============= Generic Request types =============

	struct activate_response {
		std::set<std::string> implements{};
	};

	struct empty_type {};

	struct error_response {
		// Not exposed in response, but used to set the http status
		int status{};
		std::string error{};
	};
} // namespace docker_plugin