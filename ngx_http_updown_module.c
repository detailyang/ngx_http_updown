/*
* @Author: detailyang
* @Date:   2015-10-24 10:36:19
* @Last modified by:   detailyang
* @Last modified time: 2016-05-10T17:10:52+08:00
*/
#include "ngx_http_updown_module.h"

static ngx_int_t ngx_http_updown_pre_conf(ngx_conf_t *cf);
static char *ngx_http_updown_updown_default_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_updown_down_code_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_updown_up_code_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_updown_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_updown_create_loc_conf(ngx_conf_t *cf);
static ngx_http_updown_loc_conf_t *ngx_http_updown_insert_loc_conf(void *conf);
static ngx_int_t ngx_http_updown_handler(ngx_http_request_t *req);
static ngx_int_t ngx_http_updown_module_init(ngx_cycle_t *cycle);
static char *ngx_http_updown_file_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_updown_sync_to_file(ngx_http_request_t *req,
                                              ngx_http_updown_loc_conf_t *ulcf);
static ngx_int_t ngx_http_updown_sync_from_file(ngx_http_updown_loc_conf_t *ulcf,
                                              ngx_log_t *log);
static void ngx_http_updown_exit_process(ngx_cycle_t *cycle);

static ngx_command_t ngx_http_updown_commands[] = {
  {
    ngx_string("updown"),
    NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
    ngx_http_updown_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL },
  {
    ngx_string("up_code"),
    NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_http_updown_up_code_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_updown_loc_conf_t, up_code),
    NULL },
  {
    ngx_string("down_code"),
    NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_http_updown_down_code_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_updown_loc_conf_t, down_code),
    NULL },
  {
    ngx_string("updown_default"),
    NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_http_updown_updown_default_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_updown_loc_conf_t, updown_default),
    NULL },
  {
    ngx_string("updown_file"),
    NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_http_updown_file_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_updown_loc_conf_t, updown_file),
    NULL },
  ngx_null_command
};

static ngx_http_module_t ngx_http_updown_module_ctx = {
    ngx_http_updown_pre_conf,         /* preconfiguration */
    NULL,                             /* postconfiguration */
    NULL,                             /* create main configuration */
    NULL,                             /* init main configuration */
    NULL,                             /* create server configuration */
    NULL,                             /* merge server configuration */
    ngx_http_updown_create_loc_conf,  /* create location configuration */
    NULL                              /* merge location configuration */
};

ngx_module_t ngx_http_updown_module = {
    NGX_MODULE_V1,
    &ngx_http_updown_module_ctx,   /* module context */
    ngx_http_updown_commands,      /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    ngx_http_updown_module_init,   /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    ngx_http_updown_exit_process,  /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

//0 is down, 1 is up
static ngx_atomic_t *ngx_updown_status = NULL;
static ngx_array_t *ulcfs = NULL;


static ngx_int_t
ngx_http_updown_pre_conf(ngx_conf_t *cf) {
    ulcfs = ngx_array_create(cf->pool, DEFAULT_ARRAY_SIZE, sizeof(ngx_http_updown_loc_conf_t));
    if (ulcfs == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

static ngx_http_updown_loc_conf_t*
ngx_http_updown_find_loc_conf(void *conf) {
    ngx_uint_t i;
    ngx_http_updown_loc_conf_t *ulcf, *value;

    value = ulcfs->elts;
    for (i = 0; i < ulcfs->nelts; i++) {
        ulcf = (ngx_http_updown_loc_conf_t *)&value[i];
        if (ulcf->cf == conf) {
            return ulcf;
        }
    }

    return NULL;
}

static ngx_http_updown_loc_conf_t *
ngx_http_updown_insert_loc_conf(void *conf) {
    ngx_http_updown_loc_conf_t *ulcf;

    ulcf = ngx_array_push(ulcfs);
    if (ulcf == NULL) {
        return NULL;
    }
    ((ngx_http_updown_loc_conf_t *)conf)->index = ulcf->index = ulcfs->nelts - 1;
    ulcf->cf = conf;
    ulcf->updown_file_fd = NGX_CONF_UNSET;
    ulcf->up_code = NGX_CONF_UNSET;
    ulcf->down_code = NGX_CONF_UNSET;
    ulcf->updown_file.len = NGX_CONF_UNSET_SIZE;
    ulcf->updown_file.data = NGX_CONF_UNSET_PTR;
    ulcf->updown_file_fd = NGX_CONF_UNSET;
    ulcf->index = NGX_CONF_UNSET;
    ulcf->updown_default = NGX_CONF_UNSET;

    return ulcf;
}

static void *
ngx_http_updown_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_updown_loc_conf_t *ulcf;

    ulcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_updown_loc_conf_t));
    if (ulcf == NULL) {
        return NGX_CONF_ERROR;
    }
    ulcf->up_code = NGX_CONF_UNSET;
    ulcf->down_code = NGX_CONF_UNSET;
    ulcf->updown_file.len = NGX_CONF_UNSET_SIZE;
    ulcf->updown_file.data = NGX_CONF_UNSET_PTR;
    ulcf->updown_file_fd = NGX_CONF_UNSET;
    ulcf->index = NGX_CONF_UNSET;
    ulcf->updown_default = NGX_CONF_UNSET;

    return ulcf;
}

static char *
ngx_http_updown_up_code_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_updown_loc_conf_t *ulcf=conf;
    char *rc;

    rc = ngx_conf_set_num_slot(cf, cmd, conf);
    if (rc != NGX_CONF_OK) {
        return rc;
    }

    ulcf = ngx_http_updown_find_loc_conf(conf);
    if (ulcf == NULL) {
        ulcf = ngx_http_updown_insert_loc_conf(conf);
        if (ulcf == NULL ) {
            return NGX_CONF_ERROR;
        }
    }
    ulcf->up_code = ((ngx_http_updown_loc_conf_t *)conf)->up_code;

    return rc;
}

static char *
ngx_http_updown_updown_default_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_updown_loc_conf_t *ulcf=conf;
    ngx_str_t                  *value;

    value = cf->args->elts;
    if (ngx_strncmp(value[1].data, "up", 2) == 0) {
        ulcf->updown_default = 1;
    } else if (ngx_strncmp(value[1].data, "down", 4) == 0) {
        ulcf->updown_default = 0;
    } else {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "updown: updown_default should be up or down");
        ulcf->updown_default = DEFAULT_UPDOWN_DEFAULT;
    }

    ulcf = ngx_http_updown_find_loc_conf(conf);
    if (ulcf == NULL) {
        ulcf = ngx_http_updown_insert_loc_conf(conf);
        if (ulcf == NULL ) {
            return NGX_CONF_ERROR;
        }
    }
    ulcf->updown_default = ((ngx_http_updown_loc_conf_t *)conf)->updown_default;

    return NGX_CONF_OK;
}

static char *
ngx_http_updown_down_code_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_updown_loc_conf_t *ulcf=conf;
    char *rc;

    rc = ngx_conf_set_num_slot(cf, cmd, conf);
    if (rc != NGX_CONF_OK) {
        return rc;
    }

    ulcf = ngx_http_updown_find_loc_conf(conf);
    if (ulcf == NULL) {
        ulcf = ngx_http_updown_insert_loc_conf(conf);
        if (ulcf == NULL ) {
            return NGX_CONF_ERROR;
        }
    }
    ulcf->down_code = ((ngx_http_updown_loc_conf_t *)conf)->down_code;

    return rc;
}

static char *
ngx_http_updown_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t  *clcf;
    ngx_http_updown_loc_conf_t *ulcf=conf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_updown_handler;

    ulcf = ngx_http_updown_find_loc_conf(conf);
    if (ulcf == NULL) {
        ulcf = ngx_http_updown_insert_loc_conf(conf);
        if (ulcf == NULL ) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
};

static char *
ngx_http_updown_file_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t                         *value;
    ngx_http_updown_loc_conf_t        *ulcf=conf;

    value = cf->args->elts;

    ulcf->updown_file = value[1];
    if (ulcf->updown_file.len == NGX_CONF_UNSET_SIZE) {
        return NGX_CONF_ERROR;
    }

    if (ulcf->updown_file_fd == NGX_CONF_UNSET) {
        ulcf->updown_file_fd = ngx_open_file(value[1].data, NGX_FILE_RDWR, NGX_FILE_CREATE_OR_OPEN, 0);
        if (ulcf->updown_file_fd == NGX_INVALID_FILE) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno,
                              ngx_open_file_n " \"%s\" failed", value[1].data);
            return NGX_CONF_ERROR;
        }
        if (ngx_change_file_access(ulcf->updown_file.data,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH|S_IWOTH) == -1 ) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno,
                              ngx_change_file_access_n " \"%s\" failed", value[1].data);
            return NGX_CONF_ERROR;
        }
    }

    ulcf = ngx_http_updown_find_loc_conf(conf);
    if (ulcf == NULL) {
        ulcf = ngx_http_updown_insert_loc_conf(conf);
        if (ulcf == NULL ) {
            return NGX_CONF_ERROR;
        }
    }
    ulcf->updown_file = ((ngx_http_updown_loc_conf_t *)conf)->updown_file;
    ulcf->updown_file_fd = ((ngx_http_updown_loc_conf_t *)conf)->updown_file_fd;

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_updown_module_init(ngx_cycle_t *cycle) {
    u_char                          *shared;
    ngx_uint_t                       i;
    ngx_int_t                       updown_status;
    size_t                           size;
    ngx_shm_t                        shm;
    ngx_http_updown_loc_conf_t      *value;

    size = CACHE_LINE * (ulcfs->nelts);

    if (ngx_updown_status == NULL) {
        shm.size = size;
        shm.name.len = sizeof("nginx_shared_zone_updown");
        shm.name.data = (u_char *) "nginx_shared_zone_updown";
        shm.log = cycle->log;

        if (ngx_shm_alloc(&shm) != NGX_OK) {
          return NGX_ERROR;
        }
        shared = shm.addr;
        ngx_updown_status = (ngx_atomic_t *) (shared);
        value = ulcfs->elts;
        for (i = 0; i < ulcfs->nelts; i++ ) {
            if (value[i].updown_file.len == 0 || value[i].updown_file.len == NGX_CONF_UNSET_SIZE) {
                ngx_atomic_cmp_set(ngx_updown_status + i * CACHE_LINE,
                    *(ngx_updown_status + i * CACHE_LINE), value[i].updown_default);
            } else {
                updown_status = ngx_http_updown_sync_from_file(&value[i], cycle->log);
                if (updown_status == -1) {
                    if (ngx_write_fd(value[i].updown_file_fd,
                        value[i].updown_default == NGX_CONF_UNSET? "1" : "0", 1) == NGX_ERROR) {

                        ngx_log_error(NGX_LOG_ERR, cycle->log, 0,
                            "updown: sync file error %V", &value[i].updown_file);
                        return NGX_ERROR;
                    }
                    ngx_atomic_cmp_set(ngx_updown_status + i * CACHE_LINE,
                        *(ngx_updown_status + i * CACHE_LINE), value[i].updown_default);
                } else {
                    ngx_atomic_cmp_set(ngx_updown_status + i * CACHE_LINE,
                        *(ngx_updown_status + i * CACHE_LINE), updown_status);
                }
            }
        }
    }

    return NGX_OK;
}

static void
ngx_http_updown_exit_process(ngx_cycle_t *cycle) {
    ngx_http_updown_loc_conf_t      *value;

    value = ulcfs->elts;
    // TODO: close file
}

static ngx_int_t
ngx_http_updown_handler_get (ngx_http_request_t *req) {
    u_char ngx_response_body[1024] = {0};
    ngx_http_updown_loc_conf_t *ulcf;
    ngx_int_t rc;

    ulcf = ngx_http_get_module_loc_conf(req, ngx_http_updown_module);

    if (*(ngx_updown_status + ulcf->index * CACHE_LINE) == (ngx_atomic_t) 0) {
        ngx_sprintf(ngx_response_body, "down");
        req->headers_out.status = (ulcf->down_code == NGX_CONF_UNSET ? DEFAULT_DOWN_CODE: ulcf->down_code);
    } else {
        ngx_sprintf(ngx_response_body, "up");
        req->headers_out.status = (ulcf->up_code == NGX_CONF_UNSET ? DEFAULT_UP_CODE : ulcf->up_code);
    }
    req->headers_out.content_length_n = ngx_strlen(ngx_response_body);;
    ngx_str_set(&req->headers_out.content_type, "text/html");
    rc = ngx_http_send_header(req);
    if (rc == NGX_ERROR || rc > NGX_OK || req->header_only) {
        return rc;
    }

    ngx_buf_t *b; b = ngx_pcalloc(req->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;
    b->pos = ngx_response_body;
    b->last = ngx_response_body + req->headers_out.content_length_n;
    b->memory = 1;
    b->last_buf = 1;

    return ngx_http_output_filter(req, &out);
}

static ngx_int_t
ngx_http_updown_handler_post(ngx_http_request_t *req) {
    u_char ngx_response_body[1024] = {0};
    ngx_http_updown_loc_conf_t *ulcf ;
    ngx_int_t rc;

    ulcf = ngx_http_get_module_loc_conf(req, ngx_http_updown_module);
    ngx_atomic_cmp_set(ngx_updown_status + ulcf->index * CACHE_LINE,
        *(ngx_updown_status + ulcf->index * CACHE_LINE), 1);
    if (ngx_http_updown_sync_to_file(req, ulcf) != NGX_OK) {
        return NGX_ERROR;
    }
    ngx_sprintf(ngx_response_body, "up");
    req->headers_out.content_length_n = ngx_strlen(ngx_response_body);;
    req->headers_out.status = 200;
    ngx_str_set(&req->headers_out.content_type, "text/html");
    rc = ngx_http_send_header(req);
    if (rc == NGX_ERROR || rc > NGX_OK || req->header_only) {
        return rc;
    }

    ngx_buf_t *b;
    b = ngx_pcalloc(req->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;
    b->pos = ngx_response_body;
    b->last = ngx_response_body + req->headers_out.content_length_n;
    b->memory = 1;
    b->last_buf = 1;

    return ngx_http_output_filter(req, &out);
}

static ngx_int_t
ngx_http_updown_handler_delete(ngx_http_request_t *req) {
    u_char ngx_response_body[1024] = {0};
    ngx_http_updown_loc_conf_t *ulcf ;
    ngx_int_t rc;

    ulcf = ngx_http_get_module_loc_conf(req, ngx_http_updown_module);
    ngx_atomic_cmp_set(ngx_updown_status + ulcf->index * CACHE_LINE,
        *(ngx_updown_status + ulcf->index * CACHE_LINE), 0);
    if (ngx_http_updown_sync_to_file(req, ulcf) != NGX_OK) {
        return NGX_ERROR;
    }
    ngx_sprintf(ngx_response_body, "down");
    req->headers_out.content_length_n = ngx_strlen(ngx_response_body);;
    req->headers_out.status = 200;
    ngx_str_set(&req->headers_out.content_type, "text/html");
    rc = ngx_http_send_header(req);
    if (rc == NGX_ERROR || rc > NGX_OK || req->header_only) {
        return rc;
    }

    ngx_buf_t *b; b = ngx_pcalloc(req->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;
    b->pos = ngx_response_body;
    b->last = ngx_response_body + req->headers_out.content_length_n;
    b->memory = 1;
    b->last_buf = 1;

    return ngx_http_output_filter(req, &out);
}

static ngx_int_t
ngx_http_updown_handler(ngx_http_request_t *req) {
    switch(req->method) {
        case NGX_HTTP_GET:
            return ngx_http_updown_handler_get(req);
        case NGX_HTTP_POST:
            return ngx_http_updown_handler_post(req);
        case NGX_HTTP_DELETE:
            return ngx_http_updown_handler_delete(req);
        default:
            return NGX_HTTP_NOT_ALLOWED;
    }

    return NGX_HTTP_NOT_ALLOWED;
}

static ngx_int_t
ngx_http_updown_sync_from_file(ngx_http_updown_loc_conf_t *ulcf, ngx_log_t *log) {
    u_char recv[1] = {0};

    if (lseek(ulcf->updown_file_fd, 0, SEEK_SET) == -1) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "updown: lseek file to begin error");
        return 0;
    }
    int n = ngx_read_fd(ulcf->updown_file_fd, recv, 1);
    if (n == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_errno,
          ngx_read_file_n " \"%V\" failed", ulcf->updown_file);
        return 0;
    }

    // file is empty
    if (n == 0) {
        return -1;
    }

    return ngx_atoi(recv, 1);
}

static ngx_int_t
ngx_http_updown_sync_to_file(ngx_http_request_t *req,
                             ngx_http_updown_loc_conf_t *ulcf) {
    if (ulcf->updown_file.len == 0 || ulcf->updown_file.len == NGX_CONF_UNSET_SIZE) {
        ngx_log_error(NGX_LOG_ALERT, req->connection->log, 0,
            "updown: unset updown_file, so skip");
        return NGX_OK;
    }
    if (lseek(ulcf->updown_file_fd, 0, SEEK_SET) == -1) {
        ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
            "updown: lseet file to begin %V", &ulcf->updown_file);
        goto fail;
    }

    if (*(ngx_updown_status + ulcf->index * CACHE_LINE) == (ngx_atomic_t) 0) {
        ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
          "updown: sync file %V to 0", &ulcf->updown_file);
        if (ngx_write_fd(ulcf->updown_file_fd, "0", 1) == NGX_ERROR) {
          ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
            "updown: sync file error %V", &ulcf->updown_file);
            goto fail;
        }
    } else {
        ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
          "updown: sync file %V to 1", &ulcf->updown_file);
        if (ngx_write_fd(ulcf->updown_file_fd, "1", 1) == NGX_ERROR) {
          ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
            "updown: sync file error %V", &ulcf->updown_file);
          goto fail;
        }
    }

    return NGX_OK;

fail:
    ngx_close_file(ulcf->updown_file_fd);
    return NGX_ERROR;
}
