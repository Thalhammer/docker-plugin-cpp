#include "serialize.h"
#include "docker-plugin-cpp/ipam/api.h"
#include "docker-plugin-cpp/network/api.h"
#include "docker-plugin-cpp/plugin.h"
#include "docker-plugin-cpp/volume/api.h"
#include "picojson.h"

namespace docker_plugin {
	template <>
	std::string to_json<empty_type>(const empty_type&) { return ""; }

	template <>
	empty_type from_json<empty_type>(const std::string&) { return {}; }

	template <>
	std::string to_json<activate_response>(const activate_response& e) {
		picojson::object obj;
		picojson::array impls;
		for (auto& x : e.implements)
			impls.push_back(picojson::value(x));
		obj["Implements"] = picojson::value(impls);
		return picojson::value(obj).serialize();
	}

	template <>
	std::string to_json<error_response>(const error_response& e) {
		picojson::object obj;
		obj["Err"] = picojson::value(e.error);
		return picojson::value(obj).serialize();
	}

	namespace {
		picojson::value convert_map(const std::unordered_map<std::string, std::string>& map) {
			picojson::object obj;
			for (auto& e : map)
				obj[e.first] = picojson::value(e.second);
			return picojson::value(obj);
		}

		void convert_map(std::unordered_map<std::string, std::string>& map, const picojson::value& val) {
			if (val.is<picojson::object>()) return;
			auto& obj = val.get<picojson::object>();
			for (auto& e : obj) {
				if (!e.second.is<std::string>()) continue;
				map.emplace(e.first, e.second.get<std::string>());
			}
		}

		std::string convert_time(std::chrono::system_clock::time_point tp) {
			char time_buf[sizeof("2000-01-01T00:00:00Z")] = {};
			auto now = std::chrono::system_clock::to_time_t(tp);
			strftime(time_buf, sizeof(time_buf), "%FT%TZ", gmtime(&now));
			return {time_buf};
		}
	} // namespace

	picojson::object parse_object(const std::string& str) {
		picojson::value val;
		auto err = picojson::parse(val, str);
		if (!err.empty()) throw std::invalid_argument(err);
		if (!val.is<picojson::object>()) throw std::invalid_argument("not a json object");
		return val.get<picojson::object>();
	}

	template <>
	volume::create_request from_json<volume::create_request>(const std::string& str) {
		auto obj = parse_object(str);
		volume::create_request res;
		if (obj.count("Name") != 0 && obj.at("Name").is<std::string>())
			res.name = obj.at("Name").get<std::string>();
		if (obj.count("Opts") != 0)
			convert_map(res.options, obj.at("Opts"));
		return res;
	}

	template <>
	volume::remove_request from_json<volume::remove_request>(const std::string& str) {
		auto obj = parse_object(str);
		volume::remove_request res;
		if (obj.count("Name") != 0 && obj.at("Name").is<std::string>())
			res.name = obj.at("Name").get<std::string>();
		return res;
	}

	template <>
	volume::mount_request from_json<volume::mount_request>(const std::string& str) {
		auto obj = parse_object(str);
		volume::mount_request res;
		if (obj.count("Name") != 0 && obj.at("Name").is<std::string>())
			res.name = obj.at("Name").get<std::string>();
		if (obj.count("ID") != 0 && obj.at("ID").is<std::string>())
			res.id = obj.at("ID").get<std::string>();
		return res;
	}

	picojson::value to_json_raw(const volume::volume_info& e) {
		picojson::object obj;
		obj["Name"] = picojson::value(e.name);
		obj["Mountpoint"] = picojson::value(e.mountpoint);
		obj["CreatedAt"] = picojson::value(convert_time(e.created_at));
		obj["Status"] = convert_map(e.status);
		return picojson::value(obj);
	}

	template <>
	std::string to_json<volume::mount_response>(const volume::mount_response& e) {
		picojson::object obj;
		obj["Mountpoint"] = picojson::value(e.mountpoint);
		return picojson::value(obj).serialize();
	}

	template <>
	volume::unmount_request from_json<volume::unmount_request>(const std::string& str) {
		auto obj = parse_object(str);
		volume::unmount_request res;
		if (obj.count("Name") != 0 && obj.at("Name").is<std::string>())
			res.name = obj.at("Name").get<std::string>();
		if (obj.count("ID") != 0 && obj.at("ID").is<std::string>())
			res.id = obj.at("ID").get<std::string>();
		return res;
	}

	template <>
	volume::path_request from_json<volume::path_request>(const std::string& str) {
		auto obj = parse_object(str);
		volume::path_request res;
		if (obj.count("Name") != 0 && obj.at("Name").is<std::string>())
			res.name = obj.at("Name").get<std::string>();
		return res;
	}

	template <>
	std::string to_json<volume::path_response>(const volume::path_response& e) {
		picojson::object obj;
		obj["Mountpoint"] = picojson::value(e.mountpoint);
		return picojson::value(obj).serialize();
	}

	template <>
	volume::get_request from_json<volume::get_request>(const std::string& str) {
		auto obj = parse_object(str);
		volume::get_request res;
		if (obj.count("Name") != 0 && obj.at("Name").is<std::string>())
			res.name = obj.at("Name").get<std::string>();
		return res;
	}

	template <>
	std::string to_json<volume::get_response>(const volume::get_response& e) {
		picojson::object obj;
		obj["Volume"] = to_json_raw(e.volume);
		return picojson::value(obj).serialize();
	}

	template <>
	std::string to_json<volume::list_response>(const volume::list_response& e) {
		picojson::array arr;
		for (auto& x : e.volumes)
			arr.push_back(to_json_raw(x));
		picojson::object obj;
		obj["Volumes"] = picojson::value(arr);
		return picojson::value(obj).serialize();
	}

	template <>
	std::string to_json<volume::capabilities_response>(const volume::capabilities_response& e) {
		picojson::object caps;
		caps["Scope"] = picojson::value(e.capabilities.scope);
		picojson::object obj;
		obj["Capabilities"] = picojson::value(caps);
		return picojson::value(obj).serialize();
	}

	template <>
	std::string to_json<network::capabilities_response>(const network::capabilities_response& e) {
		picojson::object obj;
		obj["Scope"] = picojson::value(e.scope);
		obj["ConnectivityScope"] = picojson::value(e.connectivity_scope);
		return picojson::value(obj).serialize();
	}

	namespace {
		std::vector<network::ipam_data> parse_ipam_data(const picojson::value& val) {
			if (!val.is<picojson::array>()) return {};
			auto& arr = val.get<picojson::array>();
			std::vector<network::ipam_data> res;
			for (auto& e : arr) {
				if (!e.is<picojson::object>()) continue;
				auto& obj = e.get<picojson::object>();
				network::ipam_data data;
				if (obj.count("AddressSpace") != 0 && obj.at("AddressSpace").is<std::string>())
					data.address_space = obj.at("AddressSpace").get<std::string>();
				if (obj.count("Pool") != 0 && obj.at("Pool").is<std::string>())
					data.pool = obj.at("Pool").get<std::string>();
				if (obj.count("Gateway") != 0 && obj.at("Gateway").is<std::string>())
					data.gateway = obj.at("Gateway").get<std::string>();
				if (obj.count("AuxAddresses") != 0)
					convert_map(data.aux_addresses, obj.at("AuxAddresses"));
				if (!data.address_space.empty() || !data.pool.empty() || !data.gateway.empty() || !data.aux_addresses.empty())
					res.push_back(std::move(data));
			}
			return res;
		}
	} // namespace

	template <>
	network::create_network_request from_json<network::create_network_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::create_network_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("Options") != 0)
			convert_map(res.options, obj.at("Options"));
		if (obj.count("IPv4Data") != 0) res.ipv4_data = parse_ipam_data(obj.at("IPv4Data"));
		if (obj.count("IPv4Data") != 0) res.ipv4_data = parse_ipam_data(obj.at("IPv4Data"));
		return res;
	}

	template <>
	network::allocate_network_request from_json<network::allocate_network_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::allocate_network_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("Options") != 0)
			convert_map(res.options, obj.at("Options"));
		if (obj.count("IPv4Data") != 0) res.ipv4_data = parse_ipam_data(obj.at("IPv4Data"));
		if (obj.count("IPv4Data") != 0) res.ipv4_data = parse_ipam_data(obj.at("IPv4Data"));
		return res;
	}

	template <>
	std::string to_json<network::allocate_network_response>(const network::allocate_network_response& e) {
		picojson::object obj;
		obj["Options"] = convert_map(e.options);
		return picojson::value(obj).serialize();
	}

	template <>
	network::delete_network_request from_json<network::delete_network_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::delete_network_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		return res;
	}

	template <>
	network::free_network_request from_json<network::free_network_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::free_network_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		return res;
	}

	namespace {
		network::endpoint_interface parse_endpoint_interface(const picojson::value& val) {
			network::endpoint_interface res;
			if (!val.is<picojson::object>()) return res;
			auto& obj = val.get<picojson::object>();
			if (obj.count("Address") != 0 && obj.at("Address").is<std::string>())
				res.ipv4_address = obj.at("Address").get<std::string>();
			if (obj.count("AddressIPv6") != 0 && obj.at("AddressIPv6").is<std::string>())
				res.ipv6_address = obj.at("AddressIPv6").get<std::string>();
			if (obj.count("MacAddress") != 0 && obj.at("MacAddress").is<std::string>())
				res.mac_address = obj.at("MacAddress").get<std::string>();
			return res;
		}

		picojson::value serialize_endpoint_interface(const network::endpoint_interface& ei) {
			picojson::object obj;
			obj["Address"] = picojson::value(ei.ipv4_address);
			obj["AddressIPv6"] = picojson::value(ei.ipv6_address);
			obj["MacAddress"] = picojson::value(ei.mac_address);
			return picojson::value(obj);
		}
	} // namespace

	template <>
	network::create_endpoint_request from_json<network::create_endpoint_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::create_endpoint_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("EndpointID") != 0 && obj.at("EndpointID").is<std::string>())
			res.endpoint_id = obj.at("EndpointID").get<std::string>();
		if (obj.count("Interface") != 0) res.interface = parse_endpoint_interface(obj.at("Interface"));
		if (obj.count("Options") != 0) convert_map(res.options, obj.at("Options"));
		return res;
	}

	template <>
	std::string to_json<network::create_endpoint_response>(const network::create_endpoint_response& e) {
		picojson::object obj;
		if (!e.interface.empty()) obj["Interface"] = serialize_endpoint_interface(e.interface);
		return picojson::value(obj).serialize();
	}

	template <>
	network::delete_endpoint_request from_json<network::delete_endpoint_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::delete_endpoint_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("EndpointID") != 0 && obj.at("EndpointID").is<std::string>())
			res.endpoint_id = obj.at("EndpointID").get<std::string>();
		return res;
	}

	template <>
	network::info_request from_json<network::info_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::info_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("EndpointID") != 0 && obj.at("EndpointID").is<std::string>())
			res.endpoint_id = obj.at("EndpointID").get<std::string>();
		return res;
	}

	template <>
	std::string to_json<network::info_response>(const network::info_response& e) {
		picojson::object obj;
		obj["Value"] = convert_map(e.value);
		return picojson::value(obj).serialize();
	}

	template <>
	network::join_request from_json<network::join_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::join_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("EndpointID") != 0 && obj.at("EndpointID").is<std::string>())
			res.endpoint_id = obj.at("EndpointID").get<std::string>();
		if (obj.count("SandboxKey") != 0 && obj.at("SandboxKey").is<std::string>())
			res.sandbox_key = obj.at("SandboxKey").get<std::string>();
		if (obj.count("Options") != 0) convert_map(res.options, obj.at("Options"));
		return res;
	}

	template <>
	std::string to_json<network::join_response>(const network::join_response& e) {
		picojson::object obj;
		picojson::object ifname;
		ifname["SrcName"] = picojson::value(e.interface_name.src_name);
		ifname["DstPrefix"] = picojson::value(e.interface_name.dst_prefix);
		obj["InterfaceName"] = picojson::value(ifname);
		obj["Gateway"] = picojson::value(e.gateway_ipv4);
		obj["GatewayIPv6"] = picojson::value(e.gateway_ipv6);
		obj["DisableGatewayService"] = picojson::value(e.disable_gateway_service);
		picojson::array routes;
		for (auto& r : e.static_routes) {
			picojson::object route;
			route["Destination"] = picojson::value(r.destination);
			route["RouteType"] = picojson::value(int64_t(r.route_type));
			route["NextHop"] = picojson::value(r.next_hop);
			routes.push_back(picojson::value(route));
		}
		obj["StaticRoutes"] = picojson::value(routes);
		return picojson::value(obj).serialize();
	}

	template <>
	network::leave_request from_json<network::leave_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::leave_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("EndpointID") != 0 && obj.at("EndpointID").is<std::string>())
			res.endpoint_id = obj.at("EndpointID").get<std::string>();
		return res;
	}

	template <>
	network::discovery_notification from_json<network::discovery_notification>(const std::string& str) {
		auto obj = parse_object(str);
		network::discovery_notification res;
		if (obj.count("DiscoveryType") != 0 && obj.at("DiscoveryType").is<int64_t>())
			res.discovery_type = static_cast<network::discovery_notification::type>(obj.at("DiscoveryType").get<int64_t>());
		if (obj.count("DiscoveryData") != 0 && obj.at("DiscoveryData").is<picojson::object>()) {
			auto& data = obj.at("DiscoveryData").get<picojson::object>();
			switch (res.discovery_type) {
			default: return res;
			case network::discovery_notification::type::node: {
				if (data.count("Address") != 0 && data.at("Address").is<std::string>())
					res.node_info.address = data.at("Address").get<std::string>();
				if (data.count("self") != 0 && data.at("self").is<bool>())
					res.node_info.self = data.at("self").get<bool>();
			} break;
			}
		}
		return res;
	}

	template <>
	network::program_external_connectivity_request from_json<network::program_external_connectivity_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::program_external_connectivity_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("EndpointID") != 0 && obj.at("EndpointID").is<std::string>())
			res.endpoint_id = obj.at("EndpointID").get<std::string>();
		if (obj.count("Options") != 0) convert_map(res.options, obj.at("Options"));
		return res;
	}

	template <>
	network::revoke_external_connectivity_request from_json<network::revoke_external_connectivity_request>(const std::string& str) {
		auto obj = parse_object(str);
		network::revoke_external_connectivity_request res;
		if (obj.count("NetworkID") != 0 && obj.at("NetworkID").is<std::string>())
			res.network_id = obj.at("NetworkID").get<std::string>();
		if (obj.count("EndpointID") != 0 && obj.at("EndpointID").is<std::string>())
			res.endpoint_id = obj.at("EndpointID").get<std::string>();
		return res;
	}

	template <>
	std::string to_json<ipam::capabilities_response>(const ipam::capabilities_response& e) {
		picojson::object obj;
		obj["RequiresMACAddress"] = picojson::value(e.requires_mac_address);
		return picojson::value(obj).serialize();
	}

	template <>
	std::string to_json<ipam::address_spaces_response>(const ipam::address_spaces_response& e) {
		picojson::object obj;
		obj["LocalDefaultAddressSpace"] = picojson::value(e.local_default_address_space);
		obj["GlobalDefaultAddressSpace"] = picojson::value(e.local_default_address_space);
		return picojson::value(obj).serialize();
	}

	template <>
	ipam::request_pool_request from_json<ipam::request_pool_request>(const std::string& str) {
		auto obj = parse_object(str);
		ipam::request_pool_request res;
		if (obj.count("AddressSpace") != 0 && obj.at("AddressSpace").is<std::string>())
			res.address_space = obj.at("AddressSpace").get<std::string>();
		if (obj.count("Pool") != 0 && obj.at("Pool").is<std::string>())
			res.pool = obj.at("Pool").get<std::string>();
		if (obj.count("SubPool") != 0 && obj.at("SubPool").is<std::string>())
			res.sub_pool = obj.at("SubPool").get<std::string>();
		if (obj.count("Options") != 0) convert_map(res.options, obj.at("Options"));
		if (obj.count("V6") != 0 && obj.at("V6").is<bool>())
			res.ipv6 = obj.at("V6").get<bool>();
		return res;
	}

	template <>
	std::string to_json<ipam::request_pool_response>(const ipam::request_pool_response& e) {
		picojson::object obj;
		obj["PoolID"] = picojson::value(e.pool_id);
		obj["Pool"] = picojson::value(e.pool);
		obj["Data"] = convert_map(e.data);
		return picojson::value(obj).serialize();
	}

	template <>
	ipam::release_pool_request from_json<ipam::release_pool_request>(const std::string& str) {
		auto obj = parse_object(str);
		ipam::release_pool_request res;
		if (obj.count("PoolID") != 0 && obj.at("PoolID").is<std::string>())
			res.pool_id = obj.at("PoolID").get<std::string>();
		return res;
	}

	template <>
	ipam::request_address_request from_json<ipam::request_address_request>(const std::string& str) {
		auto obj = parse_object(str);
		ipam::request_address_request res;
		if (obj.count("PoolID") != 0 && obj.at("PoolID").is<std::string>())
			res.pool_id = obj.at("PoolID").get<std::string>();
		if (obj.count("Address") != 0 && obj.at("Address").is<std::string>())
			res.address = obj.at("Address").get<std::string>();
		if (obj.count("Options") != 0) convert_map(res.options, obj.at("Options"));
		return res;
	}

	template <>
	std::string to_json<ipam::request_address_response>(const ipam::request_address_response& e) {
		picojson::object obj;
		obj["Address"] = picojson::value(e.address);
		obj["Data"] = convert_map(e.data);
		return picojson::value(obj).serialize();
	}

	template <>
	ipam::release_address_request from_json<ipam::release_address_request>(const std::string& str) {
		auto obj = parse_object(str);
		ipam::release_address_request res;
		if (obj.count("PoolID") != 0 && obj.at("PoolID").is<std::string>())
			res.pool_id = obj.at("PoolID").get<std::string>();
		if (obj.count("Address") != 0 && obj.at("Address").is<std::string>())
			res.address = obj.at("Address").get<std::string>();
		return res;
	}

} // namespace docker_plugin