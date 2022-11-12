#pragma once
#include "../plugin.h"
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace docker_plugin {
	namespace network {
		struct capabilities_response {
			std::string scope{};
			std::string connectivity_scope{};
		};

		struct ipam_data {
			std::string address_space{};
			std::string pool{};
			std::string gateway{};
			std::unordered_map<std::string, std::string> aux_addresses{};
		};

		struct create_network_request {
			std::string network_id{};
			std::unordered_map<std::string, std::string> options{};
			std::vector<ipam_data> ipv4_data{};
			std::vector<ipam_data> ipv6_data{};
		};

		struct allocate_network_request {
			std::string network_id{};
			std::unordered_map<std::string, std::string> options{};
			std::vector<ipam_data> ipv4_data{};
			std::vector<ipam_data> ipv6_data{};
		};

		struct allocate_network_response {
			std::unordered_map<std::string, std::string> options{};
		};

		struct delete_network_request {
			std::string network_id{};
		};

		struct free_network_request {
			std::string network_id{};
		};

		struct endpoint_interface {
			std::string ipv4_address{};
			std::string ipv6_address{};
			std::string mac_address{};

			bool empty() const noexcept { return ipv4_address.empty() && ipv6_address.empty() && mac_address.empty(); }
		};

		struct create_endpoint_request {
			std::string network_id{};
			std::string endpoint_id{};
			endpoint_interface interface {};
			std::unordered_map<std::string, std::string> options{};
		};

		struct create_endpoint_response {
			endpoint_interface interface {};
		};

		struct delete_endpoint_request {
			std::string network_id{};
			std::string endpoint_id{};
		};

		struct info_request {
			std::string network_id{};
			std::string endpoint_id{};
		};

		struct info_response {
			std::unordered_map<std::string, std::string> value{};
		};

		struct join_request {
			std::string network_id{};
			std::string endpoint_id{};
			std::string sandbox_key{};
			std::unordered_map<std::string, std::string> options{};
		};

		struct static_route {
			std::string destination{};
			int route_type{};
			std::string next_hop{};
		};

		struct join_response {
			struct {
				std::string src_name{};
				std::string dst_prefix{};
			} interface_name{};
			std::string gateway_ipv4{};
			std::string gateway_ipv6{};
			std::vector<static_route> static_routes{};
			bool disable_gateway_service{};
		};

		struct leave_request {
			std::string network_id{};
			std::string endpoint_id{};
		};

		struct discovery_notification {
			enum class type {
				node = 1
			};
			type discovery_type{};
			struct {
				std::string address;
				bool self;
			} node_info{};
		};

		struct program_external_connectivity_request {
			std::string network_id{};
			std::string endpoint_id{};
			std::unordered_map<std::string, std::string> options{};
		};

		struct revoke_external_connectivity_request {
			std::string network_id{};
			std::string endpoint_id{};
		};

		struct driver {
			virtual ~driver() = default;
			virtual capabilities_response capabilities(const empty_type&) = 0;
			virtual error_response create_network(const create_network_request& req) = 0;
			virtual allocate_network_response allocate_network(const allocate_network_request& req) = 0;
			virtual error_response delete_network(const delete_network_request& req) = 0;
			virtual error_response free_network(const free_network_request& req) = 0;
			virtual create_endpoint_response create_endpoint(const create_endpoint_request& req) = 0;
			virtual error_response delete_endpoint(const delete_endpoint_request& req) = 0;
			virtual info_response endpoint_info(const info_request& req) = 0;
			virtual join_response join(const join_request& req) = 0;
			virtual error_response leave(const leave_request& req) = 0;
			virtual error_response discover_new(const discovery_notification& req) = 0;
			virtual error_response discover_delete(const discovery_notification& req) = 0;
			virtual error_response program_external_connectivity(const program_external_connectivity_request& req) = 0;
			virtual error_response revoke_external_connectivity(const revoke_external_connectivity_request& req) = 0;
		};
	} // namespace network
} // namespace docker_plugin