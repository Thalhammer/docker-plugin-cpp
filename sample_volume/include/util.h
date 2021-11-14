#pragma once
#include <cstring>
#include <dirent.h>
#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <fstream>

class util {
public:
	static bool is_dir(const std::string& path) {
		struct stat info;
		if (stat(path.c_str(), &info) != 0) return false;
		return info.st_mode & S_IFDIR;
	}
	static bool make_dir(const std::string& path, mode_t mode) {

		return mkdir(path.c_str(), mode) == 0;
	}
	static std::string cwd() {
		std::string buf;
		buf.resize(10);
		while (getcwd(const_cast<char*>(buf.data()), buf.size()) == nullptr && errno == ERANGE) {
			buf.resize(buf.size() * 2);
			errno = 0;
		}
		if (errno != 0) throw std::system_error(std::error_code{errno, std::system_category()});
		buf.resize(strlen(buf.c_str()));
		return buf;
	}
	static bool make_dirs(const std::string& path, mode_t mode = S_IRWXU) {
        if(path.empty()) return false;
		auto pos = path.find('/', path[0] == '/' ? 1 : 0);
		while (pos != std::string::npos) {
			auto sub = path.substr(0, pos);
            if(!is_dir(sub)) {
                if(!make_dir(sub, mode)) return false;
            }
			pos = path.find('/', pos+1);
		}
        return path.back() == '/' ? true : make_dir(path, mode);
	}
	static bool loop_dir(const std::string& path, std::function<bool(const std::string& path, bool is_dir)> cb) {
		std::unique_ptr<DIR, void(*)(DIR*)> dir{opendir(path.c_str()), [](DIR* d) { closedir(d); }};
		if (!dir) return false;
		while (auto p = readdir64(dir.get())) {
			if(!cb(p->d_name, p->d_type == DT_DIR)) return false;
		}
        return true;
	}
	static bool remove(const std::string& path) {
		if (util::is_dir(path))
			return util::remove_dir(path);
		else
			return util::remove_file(path);
	}
	static bool remove_file(const std::string& path) {
		return unlink(path.c_str()) == 0;
	}
	static bool remove_dir(const std::string& path, bool recursive = true) {
		if (recursive) {
			if (!loop_dir(path, [&](const std::string& name, bool is_dir) {
                if(name == "." || name == "..") return true;
                if(is_dir) return util::remove_dir(path + "/" + name, true);
                else return util::remove_file(path + "/" + name);
			})) return false;
		}
		return rmdir(path.c_str()) == 0;
	}
    static std::chrono::system_clock::time_point create_time(const std::string& path) {
        std::ifstream in{path+".create_ts", std::ios::binary};
        if(!in) {
            struct stat info;
		    if (stat(path.c_str(), &info) == 0)
                return std::chrono::system_clock::from_time_t(info.st_mtim.tv_sec);
            return {};
        }
        uint64_t ts;
        in >> ts;
        return std::chrono::system_clock::from_time_t(ts);
    }
};