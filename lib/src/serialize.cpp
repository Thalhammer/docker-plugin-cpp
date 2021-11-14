#include "serialize.h"
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

} // namespace docker_plugin