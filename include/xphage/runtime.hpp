#pragma once
// ============================================================
// X-Phage Runtime v4.0.0
// Phases 1-3: Core runtime + Flux<T> + EventBus
// AeonCoreX Lab
// ============================================================
#include "ast.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <stdexcept>
#include <any>

namespace xp {

// ============================================================
// Flux<T> — Reactive State (Phase 2)
// Observer pattern: any write triggers registered callbacks
// ============================================================
template<typename T>
class Flux {
public:
    using Callback = std::function<void(const T&)>;

    explicit Flux(T initial = T{}) : value_(std::move(initial)) {}

    // Read
    const T& get() const {
        std::lock_guard<std::mutex> lk(mu_);
        return value_;
    }
    operator const T&() const { return get(); }

    // Write → triggers observers
    Flux& operator=(T v) {
        {
            std::lock_guard<std::mutex> lk(mu_);
            value_ = std::move(v);
        }
        notify();
        return *this;
    }

    // Subscribe to changes
    int subscribe(Callback cb) {
        std::lock_guard<std::mutex> lk(mu_);
        int id = next_id_++;
        observers_[id] = std::move(cb);
        return id;
    }

    void unsubscribe(int id) {
        std::lock_guard<std::mutex> lk(mu_);
        observers_.erase(id);
    }

    void notify() {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto& [id, cb] : observers_) cb(value_);
    }

private:
    T                                    value_;
    std::unordered_map<int, Callback>    observers_;
    mutable std::mutex                   mu_;
    int                                  next_id_ = 0;
};

// ============================================================
// EventBus — typed emit/absorb (Phase 2)
// Decouples UI from logic completely
// ============================================================
class EventBus {
public:
    using Handler = std::function<void(const std::unordered_map<std::string,std::string>&)>;

    static EventBus& global() {
        static EventBus instance;
        return instance;
    }

    // absorb: register handler
    int on(const std::string& event, Handler handler) {
        std::lock_guard<std::mutex> lk(mu_);
        int id = next_id_++;
        handlers_[event][id] = std::move(handler);
        return id;
    }

    // emit: fire event with data map
    void emit(const std::string& event,
              const std::unordered_map<std::string,std::string>& data = {}) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = handlers_.find(event);
        if (it != handlers_.end()) {
            for (auto& [id, h] : it->second) h(data);
        }
    }

    void off(const std::string& event, int id) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = handlers_.find(event);
        if (it != handlers_.end()) it->second.erase(id);
    }

private:
    std::unordered_map<std::string,
        std::unordered_map<int, Handler>> handlers_;
    std::mutex mu_;
    int next_id_ = 0;
};

// ============================================================
// Result<T, E> — error propagation (Phase 3 '?' operator)
// ============================================================
template<typename T, typename E = std::string>
class Result {
public:
    static Result ok(T v)  { Result r; r.ok_ = true;  r.val_ = std::move(v); return r; }
    static Result err(E e) { Result r; r.ok_ = false; r.err_ = std::move(e); return r; }

    bool is_ok()  const { return  ok_; }
    bool is_err() const { return !ok_; }

    const T& value()   const { if (!ok_) throw std::runtime_error(err_); return val_; }
    const E& error()   const { return err_; }
    T        unwrap()        { if (!ok_) throw std::runtime_error(err_); return std::move(val_); }

private:
    bool ok_ = false;
    T    val_{};
    E    err_{};
};

// Option<T>
template<typename T>
class Option {
public:
    static Option some(T v) { Option o; o.has_ = true; o.val_ = std::move(v); return o; }
    static Option none()    { return Option{}; }

    bool is_some() const { return  has_; }
    bool is_none() const { return !has_; }
    const T& value() const { if (!has_) throw std::runtime_error("Option::none"); return val_; }
    T unwrap_or(T def) const { return has_ ? val_ : def; }

private:
    bool has_ = false;
    T    val_{};
};

} // namespace xp

// ============================================================
// Memory cell (Shadow RAM + Global Registry)
// ============================================================
struct MemoryCell {
    std::string data;
    std::string type;
    bool        constant  = false;
    bool        is_neural = false;
};

// ============================================================
// XPhageRuntime — interpreter runtime engine
// ============================================================
class XPhageRuntime {
public:
    std::unordered_map<std::string, MemoryCell> cell_map;
    std::unordered_map<std::string, MemoryCell> global_registry;
    std::shared_ptr<FusionNode> ui_root;
    std::shared_ptr<FusionNode> current_ui_context;
    bool ui_active    = false;
    bool vulkan_ready = false;

    // Memory
    void write(const std::string& id, const std::string& val,
               const std::string& type = "string", bool is_const = false);
    void write_global(const std::string& id, const std::string& val);
    MemoryCell read(const std::string& id) const;

    // Core ops
    void launch_quantum_process(const std::string& task_name);
    void hardware_bypass(const std::string& target, const std::string& params);
    void activate_vortex();
    void activate_void_protocol();
    void establish_synapse(const std::string& id, const std::string& target_api);
    void process_matrix(const std::string& id, const std::string& size);
    void activate_chronos(const std::string& ms_str);
    void activate_ether(const std::string& target, const std::string& data_ref);

    // GPU / NPU stubs (Phase 5+)
    void init_vulkan_pipeline();
    void gpu_compute_matrix(const std::string& matrix_id);

    // Fusion UI
    void init_fusion_engine();
    void render_ui_tree();
};

// ============================================================
// Forward declarations for legacy compiler classes
// (kept for backward compat — new code uses xphage:: ns)
// Note: XPhageLLVMCompiler is fully declared in codegen_llvm.hpp
// ============================================================
class XPhageLexer {
public:
    std::vector<Token> tokenize(const std::string& source,
                                const std::string& filename = "<input>");
};

class XPhageLinker {
public:
    void link_library(const std::string& lib_name, XPhageRuntime& runtime);
};

class XPhageTranspiler {
public:
    void transpile_to_cpp(const std::vector<Token>& tokens,
                          const std::string& output_cpp);
};

