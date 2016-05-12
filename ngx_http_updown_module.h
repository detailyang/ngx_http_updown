#ifndef NGX_HTTP_UPDOWN_MODULE_H
#define NGX_HTTP_UPDOWN_MODULE_H

#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct {
    ngx_int_t index;
    ngx_int_t updown_default;
    ngx_int_t up_code;
    ngx_int_t down_code;
    ngx_str_t updown_file;
    ngx_fd_t updown_file_fd;
    void *cf;
} ngx_http_updown_loc_conf_t;

#define CACHE_LINE 128
#define DEFAULT_ARRAY_SIZE 4
#define DEFAULT_UPDOWN_DEFAULT 1
#define DEFAULT_UP_CODE 200
#define DEFAULT_DOWN_CODE 500

#endif
