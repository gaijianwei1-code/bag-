// T1.3 实现：PCD 读取 + 2D 占据投影
#include "grid_projection.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace blz {

std::vector<Vec3> load_pcd(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open pcd: " + path);

    std::string line, data_type = "ascii";
    long long point_count = -1;
    size_t field_count = 0, x_idx = 0, y_idx = 1, z_idx = 2;
    bool fields_xyz = false;
    std::streamoff data_offset = -1;

    while (std::getline(f, line)) {
        if (!line.empty() && (line.back() == '\r')) line.pop_back();
        std::istringstream ls(line);
        std::string key; ls >> key;
        if (key == "FIELDS") { std::string v; while (ls >> v) ++field_count;
            // 定位 x/y/z 索引（按出现顺序重新扫描）
            std::istringstream ls2(line); ls2 >> key; int i = 0; std::string name;
            while (ls2 >> name) {
                if (name == "x") x_idx = i;
                if (name == "y") y_idx = i;
                if (name == "z") z_idx = i;
                ++i; }
            fields_xyz = (field_count >= 3); }
        else if (key == "POINTS") { ls >> point_count; }
        else if (key == "DATA") { ls >> data_type;
            data_offset = static_cast<std::streamoff>(f.tellg());
            break; }
    }
    if (point_count < 0 || data_offset < 0 || !fields_xyz)
        throw std::runtime_error("bad pcd header (need FIELDS x y z / POINTS / DATA): " + path);

    std::vector<Vec3> out; out.reserve(static_cast<size_t>(point_count));
    if (data_type == "ascii") {
        std::string buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::istringstream ss(buf);
        std::vector<float> vals(field_count);
        for (long long p = 0; p < point_count; ++p) {
            bool ok = true;
            for (size_t i = 0; i < field_count; ++i) { if (!(ss >> vals[i])) { ok = false; break; } }
            if (!ok) break;
            out.push_back({vals[x_idx], vals[y_idx], vals[z_idx]});
        }
    } else if (data_type == "binary") {
        f.clear();
        f.seekg(data_offset);
        const size_t stride = field_count * sizeof(float);
        std::vector<char> buf(stride);
        for (long long p = 0; p < point_count; ++p) {
            f.read(buf.data(), static_cast<std::streamsize>(stride));
            if (!f) break;
            float v[3];
            std::memcpy(&v[0], buf.data() + x_idx * sizeof(float), sizeof(float));
            std::memcpy(&v[1], buf.data() + y_idx * sizeof(float), sizeof(float));
            std::memcpy(&v[2], buf.data() + z_idx * sizeof(float), sizeof(float));
            out.push_back({v[0], v[1], v[2]});
        }
    } else {
        throw std::runtime_error("unsupported DATA " + data_type + " (ascii/binary only)");
    }
    return out;
}

GridProjection project_to_grid(const std::vector<Vec3>& pts,
                               double resolution, double zmin, double zmax) {
    GridProjection g; g.resolution = resolution;
    double min_x = 1e18, min_y = 1e18, max_x = -1e18, max_y = -1e18;
    std::vector<Vec3> kept; kept.reserve(pts.size());
    for (const auto& p : pts) {
        if (p.z < zmin || p.z > zmax) continue;
        kept.push_back(p);
        min_x = std::min(min_x, static_cast<double>(p.x));
        min_y = std::min(min_y, static_cast<double>(p.y));
        max_x = std::max(max_x, static_cast<double>(p.x));
        max_y = std::max(max_y, static_cast<double>(p.y));
    }
    if (kept.empty()) { g.width = g.height = 0; return g; }
    const int w = static_cast<int>(std::ceil((max_x - min_x) / resolution)) + 1;
    const int h = static_cast<int>(std::ceil((max_y - min_y) / resolution)) + 1;
    g.min_x = min_x; g.min_y = min_y;
    g.width = w; g.height = h;
    g.occupied.assign(static_cast<size_t>(w) * h, 0);
    for (const auto& p : kept) {
        constexpr double kEps = 1e-6;
        const int cx = static_cast<int>(std::floor((p.x - min_x) / resolution + kEps));
        const int cy = static_cast<int>(std::floor((p.y - min_y) / resolution + kEps));
        if (cx < 0 || cy < 0 || cx >= w || cy >= h) continue; // 不应发生，防御
        g.occupied[static_cast<size_t>(cy) * w + cx] = 1;
    }
    g.point_count = kept.size();
    return g;
}

} // namespace blz
