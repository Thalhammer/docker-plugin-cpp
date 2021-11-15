#pragma once
#include "../plugin.h"

namespace docker_plugin::ipam {
	struct capabilities_response {
		bool requires_mac_address{};
	};

	struct address_spaces_response {
		std::string local_default_address_space{};
		std::string global_default_address_space{};
	};

	struct request_pool_request {
		std::string address_space{};
		std::string pool{};
		std::string sub_pool{};
		std::unordered_map<std::string, std::string> options{};
		bool ipv6{};
	};

	struct request_pool_response {
		std::string pool_id{};
		std::string pool{};
		std::unordered_map<std::string, std::string> data{};
	};

	struct release_pool_request {
		std::string pool_id{};
	};

	struct request_address_request {
		std::string pool_id{};
		std::string address{};
		std::unordered_map<std::string, std::string> options{};
	};

	struct request_address_response {
		std::string address{};
		std::unordered_map<std::string, std::string> data{};
	};

	struct release_address_request {
		std::string pool_id{};
		std::string address{};
	};

	struct driver {
		virtual ~driver() = default;
		virtual capabilities_response capabilities(const empty_type&) = 0;
		virtual address_spaces_response default_address_spaces(const empty_type&) = 0;
		virtual request_pool_response request_pool(const request_pool_request& req) = 0;
		virtual error_response release_pool(const release_pool_request& req) = 0;
		virtual request_address_response request_address(const request_address_request& req) = 0;
		virtual error_response release_address(const release_address_request& req) = 0;
	};
} // namespace docker_plugin::ipam