#pragma once
#include "uds_server.h"
#include <algorithm>
#include <llhttp.h>
#include <map>

namespace docker_plugin {
	struct case_insensitive_less {
		bool operator()(const std::string& lhs, const std::string& rhs) const {
			return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
												[](const char c1, const char c2) { return tolower(c1) < tolower(c2); });
		}
	};

	class http_header_set {
	public:
		using collection_type = std::multimap<std::string, std::string, case_insensitive_less>;

		collection_type& raw() noexcept { return m_headers; }
		const collection_type& raw() const noexcept { return m_headers; }

		void set(std::string key, std::string value, bool replace = true) {
			if (replace) m_headers.erase(key);
			m_headers.emplace(std::move(key), std::move(value));
		}
		const std::string& get(const std::string& key) const noexcept {
			const static std::string empty{};
			auto it = m_headers.find(key);
			if (it == m_headers.end()) return empty;
			return it->second;
		}
		bool has(const std::string& key) const noexcept { return m_headers.count(key) != 0; }
		std::pair<collection_type::const_iterator, collection_type::const_iterator> get_all(const std::string& key) const noexcept { return m_headers.equal_range(key); }
		size_t size() const noexcept { return m_headers.size(); }
		void erase(const std::string& key) { m_headers.erase(key); }
		void clear() { m_headers.clear(); }

	private:
		collection_type m_headers{};
	};

	class http_connection : public uds_connection {
		llhttp_t m_parser{};
		std::string m_buffer{};
		bool m_buffer_body{false};
		bool m_buffer_headers{false};
		http_header_set m_headers{};
		http_header_set::collection_type::iterator m_current_header{};

		int m_response_status{200};
		std::string m_response_message{"OK"};
		http_header_set m_response_headers{};
		bool m_response_headers_sent{false};
		bool m_response_chunked{false};

		static const llhttp_settings_t& get_settings() noexcept;

	protected:
		void on_read(const void* data, size_t len) override;
		void buffer_body() noexcept { m_buffer_body = true; }
		void buffer_headers() noexcept { m_buffer_headers = true; }
		const http_header_set& request_headers() const noexcept { return m_headers; }
		const std::string& body() const noexcept { return m_buffer; }

		void response_status(int status, const std::string& msg = "");
		http_header_set& response_headers() noexcept { return m_response_headers; }
		void send_headers();
		void send_data(const void* data, size_t len);
		void send_data(const std::string& data) { send_data(data.data(), data.size()); }
		void end();
		void end(const void* data, size_t len);
		void end(const std::string& data) { end(data.data(), data.size()); }

	public:
		http_connection(int sock);

		virtual int on_message_begin() noexcept { return 0; }
		virtual int on_url(llhttp_method, const std::string&) noexcept { return 0; }
		virtual int on_header_field(const std::string&) noexcept { return 0; }
		virtual int on_header_value(const std::string&) noexcept { return 0; }
		virtual int on_headers_complete() noexcept { return 0; }
		virtual int on_body(const void*, size_t) noexcept { return 0; }
		virtual int on_message_complete() noexcept { return 0; }

		static void on_connect(const std::shared_ptr<http_connection>&) {}
		static void on_disconnect(const std::shared_ptr<http_connection>&) {}
	};

	template <typename TConnection = http_connection, typename... TExtra>
	class http_server : public uds_server {
		std::tuple<TExtra...> m_extra_args;
		template <std::size_t... Is>
		std::shared_ptr<uds_connection> create_connection_with_extra_args(int socket, std::index_sequence<Is...>) {
			return std::make_shared<TConnection>(socket, std::get<Is>(m_extra_args)...);
		}

	protected:
		void on_connect(const std::shared_ptr<uds_connection>& con) override { TConnection::on_connect(std::static_pointer_cast<TConnection>(con)); }
		void on_disconnect(const std::shared_ptr<uds_connection>& con) override { TConnection::on_disconnect(std::static_pointer_cast<TConnection>(con)); }
		std::shared_ptr<uds_connection> create_connection(int socket) override {
			return create_connection_with_extra_args(socket, std::index_sequence_for<TExtra...>());
		}

	public:
		http_server(logger* log, TExtra... args)
			: uds_server(log), m_extra_args{args...} {}
		~http_server() {}
	};
} // namespace docker_plugin