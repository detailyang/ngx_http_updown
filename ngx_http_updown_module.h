#ifndef NGX_HTTP_UPDOWN_MODULE_H
#define NGX_HTTP_UPDOWN_MODULE_H

#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct {
    ngx_str_t name;
    ngx_int_t index;
    ngx_int_t last_status;
    ngx_int_t updown_default;
    ngx_int_t up_code;
    ngx_int_t down_code;
    ngx_str_t updown_file;
    ngx_str_t updown_upstream;
    void *cf;
} ngx_http_updown_loc_conf_t;

#define CACHE_LINE 128
#define DEFAULT_ARRAY_SIZE 4
#define DEFAULT_UPDOWN_DEFAULT 1
#define DEFAULT_UP_CODE 200
#define DEFAULT_DOWN_CODE 500
#define ngx_atomic_int_assign(lock, assign) ngx_atomic_cmp_set((ngx_atomic_int_t *)(lock), *(lock), (assign))

#if (NGX_HTTP_UPSTREAM_CHECK)
extern ngx_uint_t ngx_http_upstream_check_upstream_down(ngx_str_t *upstream);
#endif

#endif
