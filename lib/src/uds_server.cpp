#include "uds_server.h"
#include "docker-plugin-cpp/logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace docker_plugin {

	uds_connection::~uds_connection() {
		if (m_socket >= 0) ::close(m_socket);
	}

	void uds_connection::close() {
		if (m_socket >= 0) ::close(m_socket);
		m_socket = -1;
	}

	size_t uds_connection::write(const void* data, size_t len) {
		if (m_socket < 0) return SIZE_MAX;
		if (m_server->m_logger) m_server->m_logger->log(logger::level::debug, "[" + std::to_string(m_socket) + "] out " + std::to_string(len) + " bytes");
		auto ptr = reinterpret_cast<const uint8_t*>(data);
		auto remaining = len;
		while (remaining > 0) {
			int res = send(m_socket, ptr, remaining, 0);
			if (res < 0) return SIZE_MAX;
			remaining -= res;
			ptr += res;
		}
		return len;
	}

	uds_server::uds_server(logger* log)
		: m_logger{log} {}

	uds_server::~uds_server() {
		if (m_socket >= 0)
		{
			::close(m_socket);
		}
	}

	void uds_server::bind(const std::string& path, std::error_code& ec) {
		struct sockaddr_un addr {};
		addr.sun_family = AF_LOCAL;
		if (path.size() > sizeof(addr.sun_path))
		{
			ec = std::make_error_code(std::errc::invalid_argument);
			return;
		}
		strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));
		int s = socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
		if (s < 0)
		{
			ec = std::error_code(errno, std::system_category());
			return;
		}
		unlink(path.c_str());
		if (::bind(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0 || listen(s, 5) != 0)
		{
			ec = std::error_code(errno, std::system_category());
			::close(s);
			return;
		}
		m_socket = s;
		ec.clear();
	}

	int uds_server::run(size_t timeout_ms) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(m_socket, &set);
		auto max = m_socket;
		for (auto& e : m_connections) {
			FD_SET(e->get_fd(), &set);
			max = std::max(max, e->get_fd());
		}
		struct timeval tv {};
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		auto res = select(max + 1, &set, NULL, NULL, &tv);
		if (res < 0) return errno;
		if (res == 0) return 0;
		if (FD_ISSET(m_socket, &set)) {
			struct sockaddr_storage address;
			socklen_t addrlen;
			int new_sock = accept4(m_socket, reinterpret_cast<struct sockaddr*>(&address), &addrlen, SOCK_CLOEXEC);
			if (new_sock != -1) {
				auto con = this->create_connection(new_sock);
				if (con) {
					con->m_server = this;
					this->on_connect(con);
					log(logger::level::debug, "New socket " + std::to_string(new_sock));
					bool closed = handle_io(*con);
					if (closed) {
						log(logger::level::debug, "Closed socket " + std::to_string(new_sock));
						this->on_disconnect(con);
					} else
						m_connections.emplace_back(con);
				} else
					::close(new_sock);
			}
		}
		for (auto it = m_connections.begin(); it != m_connections.end();) {
			auto fd = (*it)->get_fd();
			if (FD_ISSET(fd, &set)) {
				auto closed = handle_io(*it->get());
				if (closed) {
					log(logger::level::debug, "Closed socket " + std::to_string(fd));
					this->on_disconnect(*it);
					it = m_connections.erase(it);
					continue;
				}
			}
			it++;
		}
		return 0;
	}

	void uds_server::log(log_level lvl, const std::string& msg) {
		if (m_logger) m_logger->log(lvl, msg);
	}

	bool uds_server::handle_io(uds_connection& con) {
		char buffer[32 * 1024];
		int res = recv(con.get_fd(), buffer, sizeof(buffer), MSG_DONTWAIT);
		if (res == 0) return true;
		if (res < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return false;
			con.close();
			return true;
		}
		log(logger::level::debug, "[" + std::to_string(con.get_fd()) + "] in " + std::to_string(res) + " bytes");
		con.on_read(buffer, res);
		return con.get_fd() < 0;
	}

} // namespace docker_plugin