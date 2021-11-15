#include "docker-plugin-cpp/plugin.h"
#include "docker-plugin-cpp/logger.h"
#include "docker-plugin-cpp/volume/api.h"
#include "http_server.h"
#include "serialize.h"
#include <csignal>
#include <cstring>
#include <iostream>

namespace docker_plugin {
	class plugin_http_connection : public http_connection {
		plugin_http_connection(const plugin_http_connection&) = delete;
		plugin_http_connection& operator=(const plugin_http_connection&) = delete;
		plugin_http_connection(plugin_http_connection&& other) = delete;
		plugin_http_connection& operator=(plugin_http_connection&& other) = delete;

		plugin* m_plugin;
		logger* m_logger;
		std::string m_url;

		template <typename TObject, typename TRequest, typename TResponse>
		void invoke_plugin_handler(TResponse (TObject::*fn)(const TRequest&), TObject* obj) {
			response_headers().set("Content-Type", "application/vnd.docker.plugins.v1.1+json");
			std::string response;
			try {
				auto req = from_json<TRequest>(body());
				auto res = (obj->*fn)(req);
				response_status(200);
				response = to_json<TResponse>(res);
			} catch (const error_response& e) {
				response_status(e.status);
				response = to_json<error_response>(e);
			} catch (const std::invalid_argument& e) {
				response_status(400);
				response = to_json<error_response>({0, e.what()});
			} catch (const std::exception& e) {
				response_status(500);
				response = to_json<error_response>({0, e.what()});
			}
			end(response);
		}

		activate_response plugin_activate(const empty_type&) {
			activate_response resp;
			if (m_plugin->m_volume_driver != nullptr) resp.implements.insert("VolumeDriver");
			return resp;
		}

	public:
		plugin_http_connection(int socket, plugin* p)
			: http_connection{socket}, m_plugin{p}, m_logger{p->m_logger}, m_url{} {}
		int on_message_begin() noexcept override {
			buffer_headers();
			buffer_body();
			return 0;
		}
		int on_url(llhttp_method method, const std::string& url) noexcept override {
			if (method != HTTP_POST) {
				if (m_logger) m_logger->log(logger::level::warning, "Get a non post request for '" + url + "'");
				response_status(405);
				end("Method not allowed");
				return 1;
			}
			m_url = url;
			return 0;
		}
		int on_message_complete() noexcept override {
			if (m_logger) m_logger->log(logger::level::info, m_url);
			if (m_url == "/Plugin.Activate") {
				this->invoke_plugin_handler(&plugin_http_connection::plugin_activate, this);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.Create") {
				this->invoke_plugin_handler(&volume::driver::create, m_plugin->m_volume_driver);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.Remove") {
				this->invoke_plugin_handler(&volume::driver::remove, m_plugin->m_volume_driver);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.Mount") {
				this->invoke_plugin_handler(&volume::driver::mount, m_plugin->m_volume_driver);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.Path") {
				this->invoke_plugin_handler(&volume::driver::path, m_plugin->m_volume_driver);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.Unmount") {
				this->invoke_plugin_handler(&volume::driver::unmount, m_plugin->m_volume_driver);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.Get") {
				this->invoke_plugin_handler(&volume::driver::get, m_plugin->m_volume_driver);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.List") {
				this->invoke_plugin_handler(&volume::driver::list, m_plugin->m_volume_driver);
			} else if (m_plugin->m_volume_driver && m_url == "/VolumeDriver.Capabilities") {
				this->invoke_plugin_handler(&volume::driver::capabilities, m_plugin->m_volume_driver);
			} else {
				// TODO: Handle Message
				response_status(404);
				end("Not found");
			}
			return 0;
		}
	};

	plugin::plugin(const std::string& driver_name, logger* log)
		: m_logger{log}, m_server{nullptr}, m_volume_driver{nullptr} {
		// Silence the dinos
		signal(SIGPIPE, SIG_IGN);
		m_server = std::make_unique<http_server<plugin_http_connection, plugin*>>(m_logger, this);
		std::error_code ec;
		m_server->bind("/run/docker/plugins/" + driver_name + ".sock", ec);
		if (ec) {
			if (m_logger) m_logger->log(logger::level::error, "Failed to bind to /run/docker/plugins/" + driver_name + ".sock: " + ec.message());
			throw std::system_error(ec);
		}
		if (m_logger) m_logger->log(logger::level::debug, "Bound plugin to uds socket /run/docker/plugins/" + driver_name + ".sock");
	}

	plugin::~plugin() {
		m_server.reset();
	}

	int plugin::run(std::chrono::milliseconds timeout) {
		return m_server->run(timeout.count());
	}
} // namespace docker_plugin