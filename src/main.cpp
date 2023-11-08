#include <array>
#include <filesystem>
#include <fstream>

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

struct ContainerInfo {
    std::string id;
    std::string name;
    std::string address;
    uint32_t pid;
    uint32_t iface_index;
    bool is_running;
};

ContainerInfo parse_config_file(const fs::path& path)
{
    if (!fs::is_regular_file(path)) {
        throw std::runtime_error(
          fmt::format("{} is not a regular file", path.generic_string()));
    }

    std::fstream config_file{path};
    config_file.exceptions(std::ios::failbit | std::ios::badbit);

    const json config_json = json::parse(config_file);

    config_file.close();

    const auto& state_json = config_json.at("State");
    const bool is_running = state_json.at("Running").get<bool>();
    const auto pid = state_json.at("Pid").get<uint32_t>();
    auto name = config_json.at("Name").get<std::string>();
    auto id = config_json.at("ID").get<std::string>();

    std::string address{};
    const auto networks = config_json.value(
      json::json_pointer("/NetworkSettings/Networks"), json{});

    const auto first = networks.items().begin();
    if (first != networks.items().end()) {
        address = first.value().value("IPAddress", std::string{});
    }

    return ContainerInfo{.id = id,
      .name = std::string(name, 1),
      .address = address,
      .pid = pid,
      .iface_index = 0,
      .is_running = is_running};
}

template <typename T>
T read_single_value_from_file(const std::string& file_name)
{
    std::ifstream input_file(file_name, std::ios::in);
    input_file.exceptions(std::ios::failbit | std::ios::badbit);

    T res{};
    input_file >> res;

    return res;
}

std::vector<ContainerInfo> list()
{
    static const fs::path container_info_path = "/var/lib/docker/containers/";
    static const fs::path proc_path = "/proc/";

    std::vector<ContainerInfo> res{};

    for (const auto& entry : fs::directory_iterator(container_info_path)) {

        if (!entry.is_directory()) {
            continue;
        }

        std::string container_id = entry.path().filename().generic_string();

        const auto config_json_file_path = entry / fs::path{"config.v2.json"};

        auto container_info = parse_config_file(config_json_file_path);

        if (container_info.is_running && container_info.pid != 0) {
            fs::path container_fs_path
              = proc_path / std::to_string(container_info.pid) / "root";
            fs::path iface_index_path
              = container_fs_path / "sys/class/net/eth0/iflink";
            auto iface_index
              = read_single_value_from_file<uint32_t>(iface_index_path);
            container_info.iface_index = iface_index;
        }

        res.push_back(container_info);
    }
    return res;
}

std::unordered_map<uint32_t, std::string> load_local_ifaces()
{
    static const fs::path ifaces_path = "/sys/class/net/";

    std::unordered_map<uint32_t, std::string> res{};

    for (const auto& entry : fs::directory_iterator(ifaces_path)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::string iface_name = entry.path().filename().generic_string();
        const auto iface_index_path = entry / fs::path{"ifindex"};

        auto iface_index
          = read_single_value_from_file<uint32_t>(iface_index_path);

        res.insert({iface_index, iface_name});
    }

    return res;
}

} // namespace

int main()
{
    try {
        const auto containers = list();
        const auto ifaces = load_local_ifaces();

        const auto max_element
          = std::max_element(containers.cbegin(), containers.cend(),
            [](const ContainerInfo& lhs, const ContainerInfo& rhs) {
                return lhs.name.size() < rhs.name.size();
            });
        const auto max_length = max_element->name.length();

        for (const auto& c : containers) {
            std::string iface_name{};
            if (ifaces.contains(c.iface_index)) {
                iface_name = ifaces.at(c.iface_index);
            }

            fmt::print("{:<{}}  {:>8}  {:<15} {}\n", c.name, max_length, c.pid,
              c.address, iface_name);
        }
    } catch (const std::exception& ex) {
        fmt::print(stderr, "ERROR: {}\n", ex.what());
    }
}
