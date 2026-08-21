// ============================================================
// xprt.h — X-Phage Runtime Library v4.0.0
// Provides: string ops, EventBus, Flux, proc, env
// Compiled as a static C library; called from LLVM IR
// AeonCoreX Lab
// ============================================================
#pragma once
#ifndef XPRT_H
#define XPRT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── XpStr: heap-managed string (ref-counted) ─────────────────
typedef struct {
    char*    data;
    int64_t  len;
    int64_t  cap;
} XpStr;

XpStr*  xprt_str_new(const char* literal);
XpStr*  xprt_str_from_int(int64_t v);
XpStr*  xprt_str_from_float(double v);
XpStr*  xprt_str_from_bool(int v);
XpStr*  xprt_str_concat(XpStr* a, XpStr* b);
XpStr*  xprt_str_concat_cstr(XpStr* a, const char* b);
XpStr*  xprt_str_format(const char* fmt, ...); // f-string helper
int64_t xprt_str_len(XpStr* s);
int     xprt_str_eq(XpStr* a, XpStr* b);
int     xprt_str_eq_cstr(XpStr* s, const char* c);
XpStr*  xprt_str_trim(XpStr* s);
XpStr*  xprt_str_upper(XpStr* s);
XpStr*  xprt_str_lower(XpStr* s);
XpStr*  xprt_str_replace(XpStr* s, XpStr* from, XpStr* to);
void    xprt_str_free(XpStr* s);
const char* xprt_str_cstr(XpStr* s);

// ── Beam (print) ──────────────────────────────────────────────
void xprt_beam_str(XpStr* s);
void xprt_beam_int(int64_t v);
void xprt_beam_float(double v);
void xprt_beam_bool(int v);
void xprt_beam_newline(void);

// ── Scan (input) ──────────────────────────────────────────────
XpStr*  xprt_scan_str(void);
int64_t xprt_scan_int(void);
double  xprt_scan_float(void);

// ── EventBus: typed emit/absorb ──────────────────────────────
typedef void (*XpHandler)(const char* event, const char* data);
void xprt_absorb(const char* event, XpHandler handler);
void xprt_emit(const char* event, const char* data);
void xprt_emit_flush(void);           // drain all pending events

// ── Flux: reactive state ──────────────────────────────────────
typedef void* XpFlux;
typedef void (*XpFluxCb)(XpFlux flux);
XpFlux  xprt_flux_new_int(int64_t v);
XpFlux  xprt_flux_new_float(double v);
XpFlux  xprt_flux_new_str(XpStr* v);
void    xprt_flux_set_int(XpFlux f, int64_t v);
void    xprt_flux_set_float(XpFlux f, double v);
void    xprt_flux_set_str(XpFlux f, XpStr* v);
int64_t xprt_flux_get_int(XpFlux f);
double  xprt_flux_get_float(XpFlux f);
XpStr*  xprt_flux_get_str(XpFlux f);
void    xprt_flux_subscribe(XpFlux f, XpFluxCb cb);
void    xprt_flux_free(XpFlux f);

// ── XpVec: dynamic array ─────────────────────────────────────
typedef struct {
    void**   data;
    int64_t  len;
    int64_t  cap;
} XpVec;

XpVec*  xprt_vec_new(void);
void    xprt_vec_push(XpVec* v, void* item);
void*   xprt_vec_get(XpVec* v, int64_t i);
int64_t xprt_vec_len(XpVec* v);
void    xprt_vec_free(XpVec* v);

// ── XpMap: string→void* hash map ─────────────────────────────
typedef void* XpMap;
XpMap   xprt_map_new(void);
void    xprt_map_set(XpMap m, const char* key, void* val);
void*   xprt_map_get(XpMap m, const char* key);
int     xprt_map_has(XpMap m, const char* key);
void    xprt_map_free(XpMap m);

// ── Process capture (proc keyword) ───────────────────────────
XpStr*  xprt_proc(const char* cmd);

// ── Environment (env keyword) ────────────────────────────────
XpStr*  xprt_env_get(const char* key);

// ── Math helpers ─────────────────────────────────────────────
int64_t xprt_min_i64(int64_t a, int64_t b);
int64_t xprt_max_i64(int64_t a, int64_t b);
int64_t xprt_clamp_i64(int64_t v, int64_t lo, int64_t hi);
double  xprt_pow(double b, double e);

// ── Chrono (chronos keyword) ─────────────────────────────────
void    xprt_chronos(int64_t ms);

// ── Bypass (system call) ─────────────────────────────────────
void    xprt_bypass(const char* cmd);

// ── GC / arena (Phase 8+) ────────────────────────────────────
void    xprt_gc_collect(void);

#ifdef __cplusplus
}
#endif
#endif // XPRT_H
