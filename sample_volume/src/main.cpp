#include <docker-plugin-cpp/volume/api.h>
#include <docker-plugin-cpp/logger.h>
#include <iostream>
#include <algorithm>
#include "util.h"

using namespace docker_plugin::volume;
using namespace docker_plugin;

struct volume_plugin : driver {
    std::string m_root{};

    error_response create(const create_request& req) override {
        std::cout << "Creating volume " << req.name << std::endl;
        if(util::is_dir(m_root + req.name)) throw error_response{409, "volume " + req.name + " already exists"};
        if(!util::make_dirs(m_root + req.name)) return { 0, "Failed to create volume directory" };
        std::ofstream out{m_root + req.name + ".create_ts", std::ios::binary};
        out << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return {};
    }

    list_response list(const empty_type&) override {
        std::cout << "Listing volumes" << std::endl;
        list_response res;
        util::loop_dir(m_root, [&](const std::string& path, bool is_dir){
            if(!is_dir || path == "." || path == "..") return true;
            res.volumes.insert({ path, m_root + path, util::create_time(m_root + path), {} });
            return true;
        });
        return res;
    }

    get_response get(const get_request& req) override {
        std::cout << "Get volume " << req.name << std::endl;
        if(!util::is_dir(m_root + req.name)) throw error_response{404, "Could not find volume"};
        return { volume_info { req.name, m_root + req.name, util::create_time(m_root + req.name), {} } };
    }

    error_response remove(const remove_request& req) override {
        std::cout << "Remove volume " << req.name << std::endl;
        if(!util::is_dir(m_root + req.name)) throw error_response {404, "Could not find volume " + req.name};
        if(!util::remove_dir(m_root + req.name, true)) return {0, "Failed to remove volume"};
        util::remove_file(m_root + req.name + ".create_ts");
        return {};
    }

    path_response path(const path_request& req) override {
        std::cout << "Get path for volume " << req.name << std::endl;
        if(util::is_dir(m_root + req.name)) return { m_root + req.name };
        throw error_response {404, "Could not find volume " + req.name};
    }

    mount_response mount(const mount_request& req) override {
        std::cout << "Mount volume " << req.name << std::endl;
        if(util::is_dir(m_root + req.name)) return { m_root + req.name };
        throw error_response {404, "Could not find volume " + req.name};
    }

    error_response unmount(const unmount_request& req) override {
        std::cout << "Unmount volume " << req.name << std::endl;
        return {};
    }

    capabilities_response capabilities(const empty_type&) override {
        capabilities_response res;
        res.capabilities.scope = "Local";
        return res;
    }

};

int main() {
    stdout_logger logger{};
    logger.min_level = logger::level::trace;
    volume_plugin my_plugin;
    my_plugin.m_root = util::cwd() + "/vols/";
    if(!util::make_dirs(my_plugin.m_root)) {
        std::cerr << "Failed to create volume directory" << std::endl;
        return -1;
    }
    docker_plugin::plugin plugin{"sample", &logger};
    plugin.register_volume(my_plugin);
    while(true) plugin.run();
    return 0;
}