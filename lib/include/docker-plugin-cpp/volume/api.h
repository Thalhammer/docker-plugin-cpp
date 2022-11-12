#pragma once
#include "../plugin.h"
#include <chrono>
#include <string>
#include <unordered_map>

namespace docker_plugin {
	namespace volume {
		struct volume_info {
			std::string name{};
			std::string mountpoint{};
			std::chrono::system_clock::time_point created_at{};
			std::unordered_map<std::string, std::string> status{};
		};
		struct create_request {
			std::string name{};
			std::unordered_map<std::string, std::string> options{};
		};
		struct remove_request {
			std::string name{};
		};
		struct mount_request {
			std::string name{};
			std::string id{};
		};
		struct mount_response {
			std::string mountpoint{};
		};
		struct unmount_request {
			std::string name{};
			std::string id{};
		};
		struct path_request {
			std::string name{};
		};
		struct path_response {
			std::string mountpoint{};
		};
		struct get_request {
			std::string name{};
		};
		struct get_response {
			volume_info volume{};
		};
		struct list_response {
			std::set<volume_info> volumes{};
		};
		struct capabilities_response {
			struct {
				std::string scope{};
			} capabilities{};
		};

		struct driver {
			virtual ~driver() = default;
			virtual error_response create(const create_request& req) = 0;
			virtual list_response list(const empty_type&) = 0;
			virtual get_response get(const get_request& req) = 0;
			virtual error_response remove(const remove_request& req) = 0;
			virtual path_response path(const path_request& req) = 0;
			virtual mount_response mount(const mount_request& req) = 0;
			virtual error_response unmount(const unmount_request& req) = 0;
			virtual capabilities_response capabilities(const empty_type&) = 0;
		};

		inline bool operator<(const volume_info& lhs, const volume_info& rhs) {
			return lhs.name < rhs.name;
		}
	} // namespace volume
} // namespace docker_plugin