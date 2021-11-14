#pragma once
#include "uds_server.h"
#include <llhttp.h>
#include <unordered_map>

namespace docker_plugin {
	class http_connection : public uds_connection {
		llhttp_t m_parser{};
		std::string m_buffer{};
		bool m_buffer_body{false};
		bool m_buffer_headers{false};
		std::unordered_multimap<std::string, std::string> m_headers{};
		std::unordered_multimap<std::string, std::string>::iterator m_current_header{};

		int m_response_status{200};
		std::string m_response_message{"OK"};
		std::unordered_multimap<std::string, std::string> m_response_headers{};
		bool m_response_headers_sent{false};

		static llhttp_settings_t get_settings();
		static llhttp_settings_t g_settings;

	protected:
		void on_read(const void* data, size_t len) override;
		void buffer_body() noexcept { m_buffer_body = true; }
		void buffer_headers() noexcept { m_buffer_headers = true; }
		const std::unordered_multimap<std::string, std::string>& get_headers() const noexcept { return m_headers; }
		const std::string& get_body() const noexcept { return m_buffer; }

		void set_status(int status, const std::string& msg = "");
		void set_header(const std::string& key, const std::string& value, bool replace = false);
		void send_headers();
		void send_data(const void* data, size_t len);
		void end();

	public:
		http_connection(int sock);

		virtual int on_message_begin() noexcept { return 0; }
		virtual int on_url(llhttp_method, const std::string&) noexcept { return 0; }
		virtual int on_header_field(const std::string&) noexcept { return 0; }
		virtual int on_header_value(const std::string&) noexcept { return 0; }
		virtual int on_headers_complete() noexcept { return 0; }
		virtual int on_body(const void*, size_t) noexcept { return 0; }
		virtual int on_message_complete() noexcept { return 0; }

		static void on_connect(std::shared_ptr<http_connection>) {}
		static void on_disconnect(std::shared_ptr<http_connection>) {}
	};

	template <typename TConnection = http_connection, typename... TExtra>
	class http_server : public uds_server {
		std::tuple<TExtra...> m_extra_args;
		template <std::size_t... Is>
		std::shared_ptr<uds_connection> create_connection_with_extra_args(int socket, std::index_sequence<Is...>) {
			return std::make_shared<TConnection>(socket, std::get<Is>(m_extra_args)...);
		}

	protected:
		void on_connect(std::shared_ptr<uds_connection> con) override { TConnection::on_connect(std::static_pointer_cast<TConnection>(con)); }
		void on_disconnect(std::shared_ptr<uds_connection> con) override { TConnection::on_disconnect(std::static_pointer_cast<TConnection>(con)); }
		std::shared_ptr<uds_connection> create_connection(int socket) override {
			return create_connection_with_extra_args(socket, std::index_sequence_for<TExtra...>());
		}

	public:
		http_server(logger* log, TExtra... args)
			: uds_server(log), m_extra_args{args...} {}
		~http_server() {}
	};
} // namespace docker_plugin