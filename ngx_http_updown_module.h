#ifndef NGX_HTTP_UPDOWN_MODULE_H
#define NGX_HTTP_UPDOWN_MODULE_H

#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct {
    int up_code;
    int down_code;
} ngx_http_updown_loc_conf_t;

#define DEFAULT_UP_CODE 200
#define DEFAULT_DOWN_CODE 500

#endif