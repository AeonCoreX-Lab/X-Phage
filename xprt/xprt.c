// ============================================================
// xprt.c — X-Phage Runtime Library v4.0.0
// Production implementation: strings, events, flux, proc, env
// AeonCoreX Lab
// ============================================================
#define _DEFAULT_SOURCE   /* useconds_t, popen */
#define _POSIX_C_SOURCE 200809L
#include "xprt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #define xprt_sleep_ms(ms) Sleep((DWORD)(ms))
#else
  #include <unistd.h>
  #define xprt_sleep_ms(ms) usleep((useconds_t)((ms)*1000))
#endif

// ─────────────────────────────────────────────────────────────
// XpStr
// ─────────────────────────────────────────────────────────────
#define XP_STR_INITIAL_CAP 16

static XpStr* xpstr_alloc(int64_t cap) {
    XpStr* s = (XpStr*)malloc(sizeof(XpStr));
    s->data = (char*)malloc((size_t)(cap + 1));
    s->data[0] = '\0';
    s->len = 0;
    s->cap = cap;
    return s;
}

XpStr* xprt_str_new(const char* lit) {
    if (!lit) lit = "";
    int64_t n = (int64_t)strlen(lit);
    XpStr* s = xpstr_alloc(n < XP_STR_INITIAL_CAP ? XP_STR_INITIAL_CAP : n);
    memcpy(s->data, lit, (size_t)n + 1);
    s->len = n;
    return s;
}

XpStr* xprt_str_from_int(int64_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)v);
    return xprt_str_new(buf);
}

XpStr* xprt_str_from_float(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", v);
    return xprt_str_new(buf);
}

XpStr* xprt_str_from_bool(int v) {
    return xprt_str_new(v ? "true" : "false");
}

XpStr* xprt_str_concat(XpStr* a, XpStr* b) {
    if (!a) return b ? xprt_str_new(b->data) : xprt_str_new("");
    if (!b) return xprt_str_new(a->data);
    int64_t n = a->len + b->len;
    XpStr* s = xpstr_alloc(n);
    memcpy(s->data, a->data, (size_t)a->len);
    memcpy(s->data + a->len, b->data, (size_t)b->len + 1);
    s->len = n;
    return s;
}

XpStr* xprt_str_concat_cstr(XpStr* a, const char* b) {
    XpStr* tmp = xprt_str_new(b);
    XpStr* res = xprt_str_concat(a, tmp);
    xprt_str_free(tmp);
    return res;
}

XpStr* xprt_str_format(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    XpStr* s = xpstr_alloc(n);
    va_start(ap, fmt);
    vsnprintf(s->data, (size_t)n + 1, fmt, ap);
    va_end(ap);
    s->len = n;
    return s;
}

int64_t xprt_str_len(XpStr* s)  { return s ? s->len : 0; }
int xprt_str_eq(XpStr* a, XpStr* b) {
    if (!a || !b) return a == b;
    return a->len == b->len && memcmp(a->data, b->data, (size_t)a->len) == 0;
}
int xprt_str_eq_cstr(XpStr* s, const char* c) {
    if (!s) return !c || !*c;
    return strcmp(s->data, c) == 0;
}
const char* xprt_str_cstr(XpStr* s) { return s ? s->data : ""; }

XpStr* xprt_str_trim(XpStr* s) {
    if (!s) return xprt_str_new("");
    const char* p = s->data;
    while (*p && isspace((unsigned char)*p)) p++;
    const char* e = s->data + s->len - 1;
    while (e > p && isspace((unsigned char)*e)) e--;
    int64_t n = (e >= p) ? (e - p + 1) : 0;
    XpStr* r = xpstr_alloc(n);
    memcpy(r->data, p, (size_t)n);
    r->data[n] = '\0';
    r->len = n;
    return r;
}

XpStr* xprt_str_upper(XpStr* s) {
    if (!s) return xprt_str_new("");
    XpStr* r = xprt_str_new(s->data);
    for (int64_t i = 0; i < r->len; i++)
        r->data[i] = (char)toupper((unsigned char)r->data[i]);
    return r;
}

XpStr* xprt_str_lower(XpStr* s) {
    if (!s) return xprt_str_new("");
    XpStr* r = xprt_str_new(s->data);
    for (int64_t i = 0; i < r->len; i++)
        r->data[i] = (char)tolower((unsigned char)r->data[i]);
    return r;
}

XpStr* xprt_str_replace(XpStr* s, XpStr* from, XpStr* to) {
    if (!s || !from || from->len == 0) return s ? xprt_str_new(s->data) : xprt_str_new("");
    // Count occurrences
    int64_t count = 0;
    const char* p = s->data;
    while ((p = strstr(p, from->data)) != NULL) { count++; p += from->len; }
    int64_t new_len = s->len + count * (to->len - from->len);
    XpStr* r = xpstr_alloc(new_len);
    const char* src = s->data;
    char* dst = r->data;
    const char* found;
    while ((found = strstr(src, from->data)) != NULL) {
        size_t prefix = (size_t)(found - src);
        memcpy(dst, src, prefix); dst += prefix;
        memcpy(dst, to->data, (size_t)to->len); dst += to->len;
        src = found + from->len;
    }
    size_t tail = strlen(src);
    memcpy(dst, src, tail + 1);
    r->len = new_len;
    return r;
}

void xprt_str_free(XpStr* s) {
    if (s) { free(s->data); free(s); }
}

// ─────────────────────────────────────────────────────────────
// Beam (output)
// ─────────────────────────────────────────────────────────────
void xprt_beam_str(XpStr* s) {
    if (s) puts(s->data); else puts("");
}
void xprt_beam_int(int64_t v)   { printf("%lld\n", (long long)v); }
void xprt_beam_float(double v)  { printf("%g\n", v); }
void xprt_beam_bool(int v)      { puts(v ? "true" : "false"); }
void xprt_beam_newline(void)    { puts(""); }

// ─────────────────────────────────────────────────────────────
// Scan (input)
// ─────────────────────────────────────────────────────────────
XpStr* xprt_scan_str(void) {
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) return xprt_str_new("");
    size_t n = strlen(buf);
    if (n > 0 && buf[n-1] == '\n') buf[--n] = '\0';
    return xprt_str_new(buf);
}

int64_t xprt_scan_int(void) {
    int64_t v = 0; scanf("%lld", (long long*)&v); return v;
}
double xprt_scan_float(void) {
    double v = 0.0; scanf("%lf", &v); return v;
}

// ─────────────────────────────────────────────────────────────
// EventBus: absorb/emit
// ─────────────────────────────────────────────────────────────
#define XP_MAX_HANDLERS 256

static struct {
    char     event[128];
    XpHandler handler;
} g_handlers[XP_MAX_HANDLERS];
static int g_handler_count = 0;

void xprt_absorb(const char* event, XpHandler h) {
    if (g_handler_count < XP_MAX_HANDLERS) {
        strncpy(g_handlers[g_handler_count].event, event, 127);
        g_handlers[g_handler_count].event[127] = '\0';
        g_handlers[g_handler_count].handler = h;
        g_handler_count++;
    }
}

void xprt_emit(const char* event, const char* data) {
    for (int i = 0; i < g_handler_count; i++) {
        if (strcmp(g_handlers[i].event, event) == 0) {
            g_handlers[i].handler(event, data ? data : "");
        }
    }
}

void xprt_emit_flush(void) { /* no-op in sync mode */ }

// ─────────────────────────────────────────────────────────────
// Flux (reactive state)
// ─────────────────────────────────────────────────────────────
typedef struct {
    int     tag;  // 0=int 1=float 2=str
    int64_t i;
    double  f;
    XpStr*  s;
    XpFluxCb subscribers[32];
    int      sub_count;
} XpFluxImpl;

static void xpflux_notify(XpFluxImpl* fl) {
    for (int i = 0; i < fl->sub_count; i++) fl->subscribers[i](fl);
}

XpFlux xprt_flux_new_int(int64_t v) {
    XpFluxImpl* f = (XpFluxImpl*)calloc(1, sizeof(XpFluxImpl));
    f->tag = 0; f->i = v; return f;
}
XpFlux xprt_flux_new_float(double v) {
    XpFluxImpl* f = (XpFluxImpl*)calloc(1, sizeof(XpFluxImpl));
    f->tag = 1; f->f = v; return f;
}
XpFlux xprt_flux_new_str(XpStr* v) {
    XpFluxImpl* f = (XpFluxImpl*)calloc(1, sizeof(XpFluxImpl));
    f->tag = 2; f->s = v ? xprt_str_new(v->data) : xprt_str_new(""); return f;
}
void xprt_flux_set_int(XpFlux f, int64_t v) {
    XpFluxImpl* fl = (XpFluxImpl*)f; fl->i = v; xpflux_notify(fl);
}
void xprt_flux_set_float(XpFlux f, double v) {
    XpFluxImpl* fl = (XpFluxImpl*)f; fl->f = v; xpflux_notify(fl);
}
void xprt_flux_set_str(XpFlux f, XpStr* v) {
    XpFluxImpl* fl = (XpFluxImpl*)f;
    xprt_str_free(fl->s);
    fl->s = v ? xprt_str_new(v->data) : xprt_str_new("");
    xpflux_notify(fl);
}
int64_t xprt_flux_get_int(XpFlux f)  { return ((XpFluxImpl*)f)->i; }
double  xprt_flux_get_float(XpFlux f){ return ((XpFluxImpl*)f)->f; }
XpStr*  xprt_flux_get_str(XpFlux f)  { return ((XpFluxImpl*)f)->s; }
void xprt_flux_subscribe(XpFlux f, XpFluxCb cb) {
    XpFluxImpl* fl = (XpFluxImpl*)f;
    if (fl->sub_count < 32) fl->subscribers[fl->sub_count++] = cb;
}
void xprt_flux_free(XpFlux f) {
    XpFluxImpl* fl = (XpFluxImpl*)f;
    if (fl->tag == 2) xprt_str_free(fl->s);
    free(fl);
}

// ─────────────────────────────────────────────────────────────
// XpVec
// ─────────────────────────────────────────────────────────────
XpVec* xprt_vec_new(void) {
    XpVec* v = (XpVec*)malloc(sizeof(XpVec));
    v->cap = 8; v->len = 0;
    v->data = (void**)malloc(sizeof(void*) * 8);
    return v;
}
void xprt_vec_push(XpVec* v, void* item) {
    if (v->len == v->cap) {
        v->cap *= 2;
        v->data = (void**)realloc(v->data, sizeof(void*) * (size_t)v->cap);
    }
    v->data[v->len++] = item;
}
void*   xprt_vec_get(XpVec* v, int64_t i) { return (i>=0&&i<v->len) ? v->data[i] : NULL; }
int64_t xprt_vec_len(XpVec* v) { return v->len; }
void    xprt_vec_free(XpVec* v) { free(v->data); free(v); }

// ─────────────────────────────────────────────────────────────
// XpMap (open-addressing hash map)
// ─────────────────────────────────────────────────────────────
typedef struct { char* key; void* val; } XpMapEntry;
typedef struct { XpMapEntry* entries; int64_t cap; int64_t len; } XpMapImpl;

static uint64_t fnv1a(const char* s) {
    uint64_t h = 14695981039346656037ULL;
    while (*s) { h ^= (uint8_t)*s++; h *= 1099511628211ULL; }
    return h;
}

XpMap xprt_map_new(void) {
    XpMapImpl* m = (XpMapImpl*)calloc(1, sizeof(XpMapImpl));
    m->cap = 16;
    m->entries = (XpMapEntry*)calloc((size_t)m->cap, sizeof(XpMapEntry));
    return m;
}
void xprt_map_set(XpMap mp, const char* key, void* val) {
    XpMapImpl* m = (XpMapImpl*)mp;
    int64_t idx = (int64_t)(fnv1a(key) % (uint64_t)m->cap);
    while (m->entries[idx].key && strcmp(m->entries[idx].key, key) != 0)
        idx = (idx + 1) % m->cap;
    if (!m->entries[idx].key) { m->entries[idx].key = strdup(key); m->len++; }
    m->entries[idx].val = val;
}
void* xprt_map_get(XpMap mp, const char* key) {
    XpMapImpl* m = (XpMapImpl*)mp;
    int64_t idx = (int64_t)(fnv1a(key) % (uint64_t)m->cap);
    while (m->entries[idx].key) {
        if (strcmp(m->entries[idx].key, key) == 0) return m->entries[idx].val;
        idx = (idx + 1) % m->cap;
    }
    return NULL;
}
int xprt_map_has(XpMap mp, const char* key) { return xprt_map_get(mp, key) != NULL; }
void xprt_map_free(XpMap mp) {
    XpMapImpl* m = (XpMapImpl*)mp;
    for (int64_t i = 0; i < m->cap; i++) if (m->entries[i].key) free(m->entries[i].key);
    free(m->entries); free(m);
}

// ─────────────────────────────────────────────────────────────
// proc / env / bypass / chronos / math
// ─────────────────────────────────────────────────────────────
XpStr* xprt_proc(const char* cmd) {
    if (!cmd) return xprt_str_new("");
    FILE* fp = popen(cmd, "r");
    if (!fp) return xprt_str_new("");
    XpStr* result = xprt_str_new("");
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
        XpStr* line = xprt_str_new(buf);
        XpStr* combined = xprt_str_concat(result, line);
        xprt_str_free(result);
        xprt_str_free(line);
        result = combined;
    }
    pclose(fp);
    // strip trailing newline
    if (result->len > 0 && result->data[result->len-1] == '\n')
        result->data[--result->len] = '\0';
    return result;
}

XpStr* xprt_env_get(const char* key) {
    const char* v = getenv(key);
    return xprt_str_new(v ? v : "");
}

void xprt_bypass(const char* cmd) {
    if (cmd) system(cmd);
}

void xprt_chronos(int64_t ms) {
    xprt_sleep_ms(ms);
}

int64_t xprt_min_i64(int64_t a, int64_t b)             { return a < b ? a : b; }
int64_t xprt_max_i64(int64_t a, int64_t b)             { return a > b ? a : b; }
int64_t xprt_clamp_i64(int64_t v, int64_t lo, int64_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
double xprt_pow(double b, double e) { return pow(b, e); }

void xprt_gc_collect(void) { /* arena GC — Phase 8 */ }

// ─────────────────────────────────────────────────────────────
// IO helpers (io stdlib)
// ─────────────────────────────────────────────────────────────
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

XpStr* xprt_io_read(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return xprt_str_new("");
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    XpStr* s = xpstr_alloc(sz);
    if (sz > 0) { fread(s->data, 1, (size_t)sz, f); s->len = sz; s->data[sz] = '\0'; }
    fclose(f);
    return s;
}

int xprt_io_write(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    fputs(content, f);
    fclose(f);
    return 1;
}

int xprt_io_append(const char* path, const char* content) {
    FILE* f = fopen(path, "ab");
    if (!f) return 0;
    fputs(content, f);
    fclose(f);
    return 1;
}

int xprt_io_exists(const char* path) {
    struct stat st; return stat(path, &st) == 0;
}

int xprt_io_is_file(const char* path) {
    struct stat st; return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int xprt_io_is_dir(const char* path) {
    struct stat st; return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int64_t xprt_io_size(const char* path) {
    struct stat st; return (stat(path, &st) == 0) ? (int64_t)st.st_size : -1;
}

int xprt_io_mkdir(const char* path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
}

XpStr* xprt_io_list_dir(const char* path) {
    DIR* d = opendir(path);
    if (!d) return xprt_str_new("");
    XpStr* result = xprt_str_new("");
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name,".")==0 || strcmp(e->d_name,"..")==0) continue;
        XpStr* name = xprt_str_new(e->d_name);
        XpStr* nl   = xprt_str_new("\n");
        XpStr* tmp  = xprt_str_concat(result, name);
        XpStr* tmp2 = xprt_str_concat(tmp, nl);
        xprt_str_free(result); xprt_str_free(name); xprt_str_free(nl); xprt_str_free(tmp);
        result = tmp2;
    }
    closedir(d);
    return result;
}

XpStr* xprt_path_join(const char* a, const char* b) {
    XpStr* sa = xprt_str_new(a);
    if (sa->len > 0 && sa->data[sa->len-1] != '/' && sa->data[sa->len-1] != '\\') {
        XpStr* sep = xprt_str_new("/");
        XpStr* tmp = xprt_str_concat(sa, sep);
        xprt_str_free(sa); xprt_str_free(sep); sa = tmp;
    }
    XpStr* sb = xprt_str_new(b);
    XpStr* r  = xprt_str_concat(sa, sb);
    xprt_str_free(sa); xprt_str_free(sb);
    return r;
}

XpStr* xprt_path_basename(const char* p) {
    const char* last = p;
    for (const char* c = p; *c; c++)
        if (*c == '/' || *c == '\\') last = c + 1;
    return xprt_str_new(last);
}

XpStr* xprt_path_dirname(const char* p) {
    size_t n = strlen(p);
    while (n > 0 && p[n-1] != '/' && p[n-1] != '\\') n--;
    if (n == 0) return xprt_str_new(".");
    XpStr* s = xpstr_alloc((int64_t)n);
    memcpy(s->data, p, n); s->data[n] = '\0'; s->len = (int64_t)n;
    return s;
}

XpStr* xprt_path_extension(const char* p) {
    const char* dot = NULL;
    for (const char* c = p; *c; c++) if (*c == '.') dot = c;
    return xprt_str_new(dot ? dot : "");
}

// ─────────────────────────────────────────────────────────────
// String helpers (string stdlib)
// ─────────────────────────────────────────────────────────────
XpStr* xprt_str_repeat(XpStr* s, int64_t n) {
    if (!s || n <= 0) return xprt_str_new("");
    XpStr* result = xpstr_alloc(s->len * n);
    for (int64_t i = 0; i < n; i++) {
        memcpy(result->data + i * s->len, s->data, (size_t)s->len);
    }
    result->len = s->len * n;
    result->data[result->len] = '\0';
    return result;
}

XpStr* xprt_str_reverse_fn(XpStr* s) {
    if (!s) return xprt_str_new("");
    XpStr* r = xprt_str_new(s->data);
    for (int64_t i = 0, j = r->len-1; i < j; i++, j--) {
        char tmp = r->data[i]; r->data[i] = r->data[j]; r->data[j] = tmp;
    }
    return r;
}

XpStr* xprt_str_pad_left(XpStr* s, int64_t width, char ch) {
    if (!s || s->len >= width) return s ? xprt_str_new(s->data) : xprt_str_new("");
    int64_t pad = width - s->len;
    XpStr* r = xpstr_alloc(width);
    memset(r->data, ch, (size_t)pad);
    memcpy(r->data + pad, s->data, (size_t)s->len + 1);
    r->len = width;
    return r;
}

XpStr* xprt_str_pad_right(XpStr* s, int64_t width, char ch) {
    if (!s || s->len >= width) return s ? xprt_str_new(s->data) : xprt_str_new("");
    XpStr* r = xpstr_alloc(width);
    memcpy(r->data, s->data, (size_t)s->len);
    memset(r->data + s->len, ch, (size_t)(width - s->len));
    r->len = width;
    r->data[r->len] = '\0';
    return r;
}

XpStr* xprt_str_center(XpStr* s, int64_t width, char ch) {
    if (!s || s->len >= width) return s ? xprt_str_new(s->data) : xprt_str_new("");
    int64_t pad_total = width - s->len;
    int64_t pad_left  = pad_total / 2;
    int64_t pad_right = pad_total - pad_left;
    XpStr* r = xpstr_alloc(width);
    memset(r->data, ch, (size_t)pad_left);
    memcpy(r->data + pad_left, s->data, (size_t)s->len);
    memset(r->data + pad_left + s->len, ch, (size_t)pad_right);
    r->len = width;
    r->data[r->len] = '\0';
    return r;
}

int64_t xprt_str_find(XpStr* s, XpStr* sub) {
    if (!s || !sub) return -1;
    const char* p = strstr(s->data, sub->data);
    return p ? (int64_t)(p - s->data) : -1;
}

XpStr* xprt_str_split(XpStr* s, XpStr* delim) {
    if (!s || !delim) return xprt_str_new("");
    XpStr* result = xprt_str_new("");
    const char* src = s->data;
    const char* found;
    while ((found = strstr(src, delim->data)) != NULL) {
        int64_t n = (int64_t)(found - src);
        XpStr* part = xpstr_alloc(n);
        memcpy(part->data, src, (size_t)n);
        part->data[n] = '\0'; part->len = n;
        XpStr* nl = xprt_str_new("\n");
        XpStr* t1 = xprt_str_concat(result, part);
        XpStr* t2 = xprt_str_concat(t1, nl);
        xprt_str_free(result); xprt_str_free(part); xprt_str_free(nl); xprt_str_free(t1);
        result = t2;
        src = found + delim->len;
    }
    XpStr* tail = xprt_str_new(src);
    XpStr* r = xprt_str_concat(result, tail);
    xprt_str_free(result); xprt_str_free(tail);
    return r;
}

// ─────────────────────────────────────────────────────────────
// OS helpers (os stdlib)
// ─────────────────────────────────────────────────────────────
#include <time.h>

XpStr* xprt_os_platform(void) {
#if defined(__ANDROID__)
    return xprt_str_new("android");
#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    return xprt_str_new("ios");
#elif defined(__APPLE__)
    return xprt_str_new("macos");
#elif defined(_WIN32)
    return xprt_str_new("windows");
#elif defined(__EMSCRIPTEN__)
    return xprt_str_new("web");
#else
    return xprt_str_new("linux");
#endif
}

XpStr* xprt_os_arch(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
    return xprt_str_new("arm64");
#elif defined(__x86_64__) || defined(_M_X64)
    return xprt_str_new("x86_64");
#elif defined(__wasm__)
    return xprt_str_new("wasm32");
#elif defined(__riscv)
    return xprt_str_new("riscv64");
#else
    return xprt_str_new("unknown");
#endif
}

int64_t xprt_os_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
}

XpStr* xprt_os_date(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return xprt_str_new(buf);
}

XpStr* xprt_os_time_str(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    return xprt_str_new(buf);
}

XpStr* xprt_os_datetime(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return xprt_str_new(buf);
}

// ─────────────────────────────────────────────────────────────
// Math helpers
// ─────────────────────────────────────────────────────────────
double xprt_lerp(double a, double b, double t) { return a + t * (b - a); }
double xprt_smoothstep(double e0, double e1, double x) {
    double t = xprt_clamp_i64((int64_t)((x-e0)/(e1-e0)), 0, 1);
    double tf = (double)t;
    return tf * tf * (3.0 - 2.0 * tf);
}
double xprt_map_range(double v, double in_lo, double in_hi, double out_lo, double out_hi) {
    return out_lo + (v - in_lo) * (out_hi - out_lo) / (in_hi - in_lo);
}
double xprt_to_rad(double deg) { return deg * 3.14159265358979323846 / 180.0; }
double xprt_to_deg(double rad) { return rad * 180.0 / 3.14159265358979323846; }
double xprt_sign(double x)     { return (x > 0) - (x < 0); }
double xprt_fract(double x)    { return x - floor(x); }

// ─────────────────────────────────────────────────────────────
// Console helpers
// ─────────────────────────────────────────────────────────────
void xprt_print(const char* msg)   { fputs(msg, stdout); fflush(stdout); }
void xprt_println(const char* msg) { puts(msg); }
void xprt_eprint(const char* msg)  { fputs(msg, stderr); fflush(stderr); }
void xprt_eprintln(const char* msg){ fputs(msg, stderr); fputc('\n', stderr); fflush(stderr); }
XpStr* xprt_input(const char* prompt) {
    if (prompt && *prompt) { fputs(prompt, stdout); fflush(stdout); }
    return xprt_scan_str();
}
