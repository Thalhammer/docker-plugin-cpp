#pragma once
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace docker_plugin {
	class uds_server;
	class logger;

	class uds_connection {
		uds_connection(const uds_connection&) = delete;
		uds_connection& operator=(const uds_connection&) = delete;
		uds_connection(uds_connection&& other) = delete;
		uds_connection& operator=(uds_connection&& other) = delete;

		int m_socket;
		uds_server* m_server;
		int get_fd() const noexcept { return m_socket; }
		bool handle_io();
		friend class uds_server;

	protected:
		size_t write(const void* data, size_t len);
		void close();
		virtual void on_read(const void* data, size_t len) = 0;

	public:
		uds_connection(int sock) : m_socket{sock}, m_server{nullptr} {}
		virtual ~uds_connection();
	};

	class uds_server {
		uds_server(const uds_server&) = delete;
		uds_server(uds_server&&) = delete;
		uds_server& operator=(const uds_server&) = delete;
		uds_server& operator=(uds_server&&) = delete;

		int m_socket{-1};
		logger* m_logger{};
		std::vector<std::shared_ptr<uds_connection>> m_connections{};
		friend class uds_connection;

	protected:
		virtual void on_connect(std::shared_ptr<uds_connection>) = 0;
		virtual void on_disconnect(std::shared_ptr<uds_connection>) = 0;
		virtual std::shared_ptr<uds_connection> create_connection(int socket) = 0;

	public:
		uds_server(logger* log);
		virtual ~uds_server();

		void bind(const std::string& path, std::error_code& ec);

		int run(size_t timeout_ms);
	};
} // namespace docker_plugin