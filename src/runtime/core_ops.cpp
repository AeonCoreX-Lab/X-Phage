// ============================================================
// X-Phage Core Operations v4.0.0
// Stdlib primitives available to generated C++ programs
// AeonCoreX Lab
// ============================================================
#include "xphage/runtime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <functional>
#include <sys/stat.h>

// ── String ops ────────────────────────────────────────────────
std::string str_trim(std::string s) {
    size_t st = s.find_first_not_of(" \t\r\n");
    size_t en = s.find_last_not_of(" \t\r\n");
    return (st == std::string::npos) ? "" : s.substr(st, en - st + 1);
}
std::string str_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper); return s;
}
std::string str_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s;
}
long long str_len(const std::string& s) { return (long long)s.size(); }
std::string str_replace(std::string s, const std::string& f, const std::string& t) {
    size_t p = 0;
    while ((p = s.find(f, p)) != std::string::npos) { s.replace(p, f.size(), t); p += t.size(); }
    return s;
}
bool str_contains(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }
bool str_starts_with(const std::string& s, const std::string& p) { return s.size()>=p.size() && s.substr(0,p.size())==p; }
bool str_ends_with(const std::string& s, const std::string& sf) { return s.size()>=sf.size() && s.substr(s.size()-sf.size())==sf; }
long long str_to_int(const std::string& s)   { return std::stoll(s); }
double    str_to_float(const std::string& s) { return std::stod(s); }
std::string int_to_str(long long v)          { return std::to_string(v); }
std::string float_to_str(double v)           { std::ostringstream ss; ss << v; return ss.str(); }

// ── Math ops ──────────────────────────────────────────────────
double xp_sqrt(double x)          { return std::sqrt(x); }
double xp_pow(double b, double e) { return std::pow(b, e); }
double xp_log(double x)           { return std::log(x); }
double xp_sin(double x)           { return std::sin(x); }
double xp_cos(double x)           { return std::cos(x); }
double xp_abs(double x)           { return std::abs(x); }
double xp_floor(double x)         { return std::floor(x); }
double xp_ceil(double x)          { return std::ceil(x); }
double xp_round(double x)         { return std::round(x); }
long long xp_min(long long a, long long b) { return std::min(a,b); }
long long xp_max(long long a, long long b) { return std::max(a,b); }

std::vector<long long> xp_range(long long start, long long end, long long step=1) {
    std::vector<long long> v;
    for (long long i=start; step>0?i<end:i>end; i+=step) v.push_back(i);
    return v;
}

// ── IO helpers ────────────────────────────────────────────────
bool io_exists(const std::string& path) {
    struct stat st{}; return stat(path.c_str(), &st) == 0;
}
std::string io_read(const std::string& path) {
    std::ifstream f(path); if (!f.is_open()) return "";
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}
bool io_write(const std::string& path, const std::string& content) {
    std::ofstream f(path); if (!f.is_open()) return false;
    f << content; return true;
}
std::string path_join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    return (a.back()=='/'||a.back()=='\\') ? (a+b) : (a+"/"+b);
}
std::string env_get(const std::string& key) {
    const char* v = std::getenv(key.c_str()); return v ? std::string(v) : "";
}

// ── Runtime memory ────────────────────────────────────────────
void XPhageRuntime::write(const std::string& id, const std::string& val,
                           const std::string& type, bool is_const) {
    auto it = cell_map.find(id);
    if (it != cell_map.end() && it->second.constant)
        throw std::runtime_error("atom '" + id + "' is immutable");
    cell_map[id] = {val, type, is_const, false};
}
void XPhageRuntime::write_global(const std::string& id, const std::string& val) {
    global_registry[id] = {val, "string", false, false};
}
MemoryCell XPhageRuntime::read(const std::string& id) const {
    auto it = cell_map.find(id);
    if (it != cell_map.end()) return it->second;
    auto git = global_registry.find(id);
    if (git != global_registry.end()) return git->second;
    return {"undefined", "void", false, false};
}
void XPhageRuntime::launch_quantum_process(const std::string& task_name) {
    std::thread([task_name](){ std::cout<<"[quantum:"<<task_name<<"] started\n"; }).detach();
}
void XPhageRuntime::hardware_bypass(const std::string& target, const std::string& params) {
    std::string cmd = target+" "+params;
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    std::cout<<"[bypass(iOS)] "<<cmd<<"\n";
#else
    std::system(cmd.c_str());
#endif
}
void XPhageRuntime::activate_vortex()         { std::cout<<"[vortex] error handler active\n"; }
void XPhageRuntime::activate_void_protocol()  { std::cout<<"[void] null protocol\n"; }
void XPhageRuntime::establish_synapse(const std::string& id, const std::string& api) {
    std::cout<<"[synapse] "<<id<<" -> "<<api<<"\n"; write(id,api,"synapse",false);
}
void XPhageRuntime::process_matrix(const std::string& id, const std::string& sz) {
    std::cout<<"[matrix] "<<id<<"["<<sz<<"]\n"; write(id,"[]","matrix",false);
}
void XPhageRuntime::activate_chronos(const std::string& ms) {
    int t=0; try{t=std::stoi(ms);}catch(...){};
    std::this_thread::sleep_for(std::chrono::milliseconds(t));
}
void XPhageRuntime::activate_ether(const std::string& tgt, const std::string& data) {
    std::cout<<"[ether] -> "<<tgt<<" data:"<<data<<"\n";
}
void XPhageRuntime::init_vulkan_pipeline()  { vulkan_ready=true; std::cout<<"[vulkan] stub\n"; }
void XPhageRuntime::gpu_compute_matrix(const std::string& m) { std::cout<<"[gpu] "<<m<<"\n"; }
void XPhageRuntime::init_fusion_engine() {
    ui_active=true; ui_root=std::make_shared<FusionNode>("root"); std::cout<<"[fusion] stub\n";
}
void XPhageRuntime::render_ui_tree() {
    if(!ui_root){std::cout<<"[fusion] no tree\n";return;}
    std::function<void(const std::shared_ptr<FusionNode>&,int)> r =
        [&](const std::shared_ptr<FusionNode>& n,int d){
            std::cout<<std::string(d*2,' ')<<"<"<<n->type<<">\n";
            for(auto&c:n->children)r(c,d+1);
        };
    r(ui_root,0);
}
