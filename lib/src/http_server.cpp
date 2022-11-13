#include "http_server.h"
#include "llhttp.h"
#include <algorithm>

namespace docker_plugin {
	const llhttp_settings_t& http_connection::get_settings() noexcept {
		static llhttp_settings_t instance = []() {
			llhttp_settings_t s{};
			llhttp_settings_init(&s);
			s.on_message_begin = [](llhttp_t* s) -> int {
				static_cast<http_connection*>(s->data)->m_buffer.clear();
				return static_cast<http_connection*>(s->data)->on_message_begin();
			};
			s.on_url = [](llhttp_t* s, const char* at, size_t length) -> int {
				static_cast<http_connection*>(s->data)->m_buffer.append(at, length);
				return 0;
			};
			s.on_url_complete = [](llhttp_t* s) -> int {
				auto res = static_cast<http_connection*>(s->data)->on_url(static_cast<llhttp_method>(s->method), static_cast<http_connection*>(s->data)->m_buffer);
				static_cast<http_connection*>(s->data)->m_buffer.clear();
				return res;
			};
			s.on_header_field = [](llhttp_t* s, const char* at, size_t length) -> int {
				static_cast<http_connection*>(s->data)->m_buffer.append(at, length);
				return 0;
			};
			s.on_header_field_complete = [](llhttp_t* s) -> int {
				auto o = static_cast<http_connection*>(s->data);
				auto res = o->on_header_field(o->m_buffer);
				if (o->m_buffer_headers) {
					std::transform(o->m_buffer.begin(), o->m_buffer.end(), o->m_buffer.begin(), ::tolower);
					o->m_current_header = o->m_headers.raw().emplace(std::move(o->m_buffer), "");
				}
				o->m_buffer.clear();
				return res;
			};
			s.on_header_value = [](llhttp_t* s, const char* at, size_t length) -> int {
				static_cast<http_connection*>(s->data)->m_buffer.append(at, length);
				return 0;
			};
			s.on_header_value_complete = [](llhttp_t* s) -> int {
				auto o = static_cast<http_connection*>(s->data);
				auto res = o->on_header_value(o->m_buffer);
				if (o->m_buffer_headers && o->m_current_header != o->m_headers.raw().end())
					o->m_current_header->second = o->m_buffer;
				o->m_buffer.clear();
				return res;
			};
			s.on_headers_complete = [](llhttp_t* s) -> int {
				static_cast<http_connection*>(s->data)->m_buffer.clear();
				return static_cast<http_connection*>(s->data)->on_headers_complete();
			};
			s.on_message_complete = [](llhttp_t* s) -> int {
				auto o = static_cast<http_connection*>(s->data);
				if (o->m_buffer_body) {
					auto res = o->on_body(o->m_buffer.data(), o->m_buffer.size());
					if (res != 0) return res;
				} else
					o->m_buffer.clear();
				return o->on_message_complete();
			};
			s.on_body = [](llhttp_t* s, const char* at, size_t length) -> int {
				auto o = static_cast<http_connection*>(s->data);
				if (o->m_buffer_body) {
					o->m_buffer.append(at, length);
					return 0;
				} else
					return o->on_body(at, length);
			};
			return s;
		}();
		return instance;
	}

	namespace {
		static const std::pair<int, const char*> http_status_map[] = {
			{100, "Continue"},
			{101, "Switching Protocols"},
			{102, "Processing"},
			{200, "OK"},
			{201, "Created"},
			{202, "Accepted"},
			{203, "Non-authoritative Information"},
			{204, "No Content"},
			{205, "Reset Content"},
			{206, "Partial Content"},
			{207, "Multi-Status"},
			{208, "Already Reported"},
			{226, "IM Used"},
			{300, "Multiple Choices"},
			{301, "Moved Permanently"},
			{302, "Found"},
			{303, "See Other"},
			{304, "Not Modified"},
			{305, "Use Proxy"},
			{307, "Temporary Redirect"},
			{308, "Permanent Redirect"},
			{400, "Bad Request"},
			{401, "Unauthorized"},
			{402, "Payment Required"},
			{403, "Forbidden"},
			{404, "Not Found"},
			{405, "Method Not Allowed"},
			{406, "Not Acceptable"},
			{407, "Proxy Authentication Required"},
			{408, "Request Timeout"},
			{409, "Conflict"},
			{410, "Gone"},
			{411, "Length Required"},
			{412, "Precondition Failed"},
			{413, "Payload Too Large"},
			{414, "Request-URI Too Long"},
			{415, "Unsupported Media Type"},
			{416, "Requested Range Not Satisfiable"},
			{417, "Expectation Failed"},
			{418, "I'm a teapot"},
			{421, "Misdirected Request"},
			{422, "Unprocessable Entity"},
			{423, "Locked"},
			{424, "Failed Dependency"},
			{426, "Upgrade Required"},
			{428, "Precondition Required"},
			{429, "Too Many Requests"},
			{431, "Request Header Fields Too Large"},
			{444, "Connection Closed Without Response"},
			{451, "Unavailable For Legal Reasons"},
			{499, "Client Closed Request"},
			{500, "Internal Server Error"},
			{501, "Not Implemented"},
			{502, "Bad Gateway"},
			{503, "Service Unavailable"},
			{504, "Gateway Timeout"},
			{505, "HTTP Version Not Supported"},
			{506, "Variant Also Negotiates"},
			{507, "Insufficient Storage"},
			{508, "Loop Detected"},
			{510, "Not Extended"},
			{511, "Network Authentication Required"},
			{599, "Network Connect Timeout Error"},
		};
	}

	void http_connection::on_read(const void* data, size_t len) {
		auto res = llhttp_execute(&m_parser, reinterpret_cast<const char*>(data), len);
		if (res != HPE_OK) {
			fprintf(stderr, "error parsing http request: %s %s\n", llhttp_errno_name(res), m_parser.reason);
			this->close();
		}
	}

	http_connection::http_connection(int sock)
		: uds_connection(sock) {
		llhttp_init(&m_parser, HTTP_REQUEST, &get_settings());
		m_parser.data = this;
	}

	void http_connection::response_status(int status, const std::string& msg) {
		m_response_status = status;
		if (msg.empty()) {
			m_response_message = "OK";
			for (auto& e : http_status_map) {
				if (e.first == status) m_response_message = e.second;
			}
		} else
			m_response_message = msg;
	}

	void http_connection::send_headers() {
		if (m_response_headers_sent) return;
		std::string headers = "HTTP/1.1 " + std::to_string(m_response_status) + " " + m_response_message + "\r\n";
		for (auto& e : m_response_headers.raw()) {
			headers += e.first + ": " + e.second + "\r\n";
		}
		headers += "\r\n";
		this->write(headers.data(), headers.size());
		m_response_headers_sent = true;
		if (m_response_headers.get("transfer-encoding") == "chunked") m_response_chunked = true;

		m_response_message.clear();
		m_response_message.shrink_to_fit();
		m_response_headers.raw().clear();
	}

	void http_connection::send_data(const void* data, size_t len) {
		if (!m_response_headers_sent) {
			if (!response_headers().has("content-length") && !response_headers().has("transfer-encoding")) {
				response_headers().set("transfer-encoding", "chunked");
			}
			this->send_headers();
		}
		if (m_response_chunked) {
			while (len != 0) {
				uint32_t small_len = std::min<size_t>(len, UINT16_MAX);
				char len_buf[16];
				int r = snprintf(len_buf, sizeof(len_buf), "%x\r\n", static_cast<unsigned int>(small_len));
				this->write(len_buf, r);
				this->write(data, small_len);
				this->write("\r\n", 2);
				len -= small_len;
			}
		} else {
			this->write(data, len);
		}
	}

	void http_connection::end() {
		if (!m_response_headers_sent) {
			if (!response_headers().has("content-length"))
				response_headers().set("content-length", "0");
			send_headers();
		}
		if (m_response_chunked) {
			this->write("0\r\n\r\n", 5);
		}

		// Clean up state and reset everything
		m_buffer.clear();
		m_buffer_body = false;
		m_buffer_headers = false;
		m_headers.clear();
		m_current_header = {};
		m_response_status = 200;
		m_response_message.clear();
		m_response_headers.clear();
		m_response_headers_sent = false;
		m_response_chunked = false;
	}

	void http_connection::end(const void* data, size_t len) {
		if (!m_response_headers_sent) {
			response_headers().set("content-length", std::to_string(len));
		}
		send_data(data, len);
		end();
	}

} // namespace docker_plugin