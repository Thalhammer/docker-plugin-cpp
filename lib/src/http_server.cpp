#include "http_server.h"

namespace docker_plugin {
	llhttp_settings_t http_connection::get_settings() {
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
			if (o->m_buffer_headers) o->m_current_header = o->m_headers.emplace(o->m_buffer, "");
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
			if (o->m_buffer_headers && o->m_current_header != o->m_headers.end())
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
	}

	llhttp_settings_t http_connection::g_settings = http_connection::get_settings();

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
		if (res == HPE_OK) {
			// Parsing ok
		} else {
			fprintf(stderr, "error parsing http request: %s %s\n", llhttp_errno_name(res), m_parser.reason);
			// TODO: Close
		}
	}

	http_connection::http_connection(int sock)
		: uds_connection(sock) {
		llhttp_init(&m_parser, HTTP_REQUEST, &g_settings);
		m_parser.data = this;
	}

	void http_connection::set_status(int status, const std::string& msg) {
		m_response_status = status;
		if (msg.empty()) {
			m_response_message = "OK";
			for (auto& e : http_status_map) {
				if (e.first == status) m_response_message = e.second;
			}
		} else
			m_response_message = msg;
	}

	void http_connection::set_header(const std::string& key, const std::string& value, bool replace) {
		if (replace) m_response_headers.erase(key);
		m_response_headers.emplace(key, value);
	}

	void http_connection::send_headers() {
		if (m_response_headers_sent) return;
		std::string headers = "HTTP/1.0 " + std::to_string(m_response_status) + " " + m_response_message + "\r\n";
		for (auto& e : m_response_headers) {
			headers += e.first + ": " + e.second + "\r\n";
		}
		headers += "\r\n";
		this->write(headers.data(), headers.size());
		m_response_headers_sent = true;
		m_response_message.clear();
		m_response_message.shrink_to_fit();
		m_response_headers.clear();
	}

	void http_connection::send_data(const void* data, size_t len) {
		this->send_headers();
		this->write(data, len);
	}

	void http_connection::end() {
		//this->close();
	}

} // namespace docker_plugin