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
static ngx_int_t ngx_http_updown_write_file(ngx_str_t *file, ngx_log_t *log, ngx_int_t status);
static void ngx_http_updown_status_copy();

static ngx_command_t ngx_http_updown_commands[] = {
  {
    ngx_string("updown"),
    NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_http_updown_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_updown_loc_conf_t, name),
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
static u_char *ngx_updown_status = NULL;
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
    ulcf->name.len = NGX_CONF_UNSET_SIZE;
    ulcf->name.data = NGX_CONF_UNSET_PTR;
    ulcf->up_code = NGX_CONF_UNSET;
    ulcf->down_code = NGX_CONF_UNSET;
    ulcf->updown_file.len = NGX_CONF_UNSET_SIZE;
    ulcf->updown_file.data = NGX_CONF_UNSET_PTR;
    ulcf->updown_default = DEFAULT_UPDOWN_DEFAULT;

    return ulcf;
}

static void *
ngx_http_updown_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_updown_loc_conf_t *ulcf;

    ulcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_updown_loc_conf_t));
    if (ulcf == NULL) {
        return NGX_CONF_ERROR;
    }
    ulcf->name.len = NGX_CONF_UNSET_SIZE;
    ulcf->name.data = NGX_CONF_UNSET_PTR;
    ulcf->up_code = NGX_CONF_UNSET;
    ulcf->down_code = NGX_CONF_UNSET;
    ulcf->updown_file.len = NGX_CONF_UNSET_SIZE;
    ulcf->updown_file.data = NGX_CONF_UNSET_PTR;
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
    ngx_http_core_loc_conf_t        *clcf;
    ngx_http_updown_loc_conf_t      *ulcf=conf;
    ngx_str_t      *value;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_updown_handler;

    ulcf = ngx_http_updown_find_loc_conf(conf);
    if (ulcf == NULL) {
        ulcf = ngx_http_updown_insert_loc_conf(conf);
        if (ulcf == NULL ) {
            return NGX_CONF_ERROR;
        }
    }
    value = cf->args->elts;
    ulcf->name.data = value[1].data;
    ulcf->name.len = value[1].len;

    return NGX_CONF_OK;
};

static char *
ngx_http_updown_file_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t                         *value;
    ngx_http_updown_loc_conf_t        *ulcf = conf;
    ngx_fd_t                           fd;

    value = cf->args->elts;
    ulcf->updown_file.data = value[1].data;
    ulcf->updown_file.len = value[1].len;
    fd = ngx_open_file(value[1].data, NGX_FILE_RDWR, NGX_FILE_CREATE_OR_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno,
                          ngx_open_file_n " \"%s\" failed", value[1].data);
        return NGX_CONF_ERROR;
    }
    ngx_close_file(fd);
    if (ngx_change_file_access(ulcf->updown_file.data,
        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH|S_IWOTH) == -1 ) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno,
          ngx_change_file_access_n " \"%s\" failed", value[1].data);

        return NGX_CONF_ERROR;
    }

    ulcf = ngx_http_updown_find_loc_conf(conf);
    if (ulcf == NULL) {
        ulcf = ngx_http_updown_insert_loc_conf(conf);
        if (ulcf == NULL ) {
            return NGX_CONF_ERROR;
        }
    }
    ulcf->updown_file.data = value[1].data;
    ulcf->updown_file.len = value[1].len;

    return NGX_CONF_OK;
}

static void
ngx_http_updown_status_copy(ngx_log_t *log) {
    ngx_int_t i;
    ngx_uint_t j;
    ngx_http_updown_loc_conf_t      *value;

    value = ulcfs->elts;
    for (i = 0; i < *(ngx_atomic_int_t *)ngx_updown_status; i ++) {
        for (j = 0; j < ulcfs->nelts; j++) {
            if (*(uint32_t *)(ngx_updown_status + CACHE_LINE * (i * 2 + 2)) ==
                ngx_murmur_hash2(value[j].name.data, value[j].name.len)) {
                value[j].last_status = *(ngx_atomic_int_t *)(ngx_updown_status + CACHE_LINE * (i * 2 + 1));
            } else {
                value[j].last_status = -1;
            }
        }
    }
}

static ngx_int_t
ngx_http_updown_module_init(ngx_cycle_t *cycle) {
    ngx_uint_t                       i;
    ngx_int_t                       updown_status;
    ngx_shm_t                       newshm, oldshm;
    ngx_http_updown_loc_conf_t      *value;

    if (ngx_updown_status != NULL) {
        ngx_http_updown_status_copy(cycle->log);
        oldshm.size = (*(ngx_atomic_t *)ngx_updown_status * 2 + 1) * CACHE_LINE;
        oldshm.name.len = sizeof("nginx_shared_zone_updown") - 1;
        oldshm.name.data = (u_char *) "nginx_shared_zone_updown";
        oldshm.log = cycle->log;
        oldshm.exists = 1;
        oldshm.addr = ngx_updown_status;
        ngx_shm_free(&oldshm);

        newshm.size = CACHE_LINE * (ulcfs->nelts * 2 + 1) + oldshm.size;
        newshm.name.len = sizeof("nginx_shared_zone_updown") - 1;
        newshm.name.data = (u_char *) "nginx_shared_zone_updown";
        newshm.log = cycle->log;
        if (ngx_shm_alloc(&newshm) != NGX_OK) {
            return NGX_ERROR;
        }

        ngx_updown_status = newshm.addr;
        ngx_atomic_int_assign(ngx_updown_status, ulcfs->nelts);
        value = ulcfs->elts;
        for (i = 0; i < ulcfs->nelts; i ++) {
            updown_status = value[i].last_status;
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "last status:%d index:%d", updown_status, i);
            if (updown_status == -1) {
                if (value[i].updown_file.len == 0 || value[i].updown_file.len == NGX_CONF_UNSET_SIZE) {
                    ngx_atomic_int_assign(
                        ngx_updown_status + CACHE_LINE * (i * 2 + 1),
                        value[i].updown_default
                    );
                } else {
                    updown_status = ngx_http_updown_sync_from_file(&value[i], cycle->log);
                    if (updown_status == -1) {
                        if (ngx_http_updown_write_file(&(value[i].updown_file), cycle->log,
                            value[i].updown_default) == NGX_FILE_ERROR) {

                            ngx_log_error(NGX_LOG_ERR, cycle->log, 0,
                                "updown: sync file error %V", &value[i].updown_file);
                            return NGX_ERROR;
                        }
                        ngx_atomic_int_assign(
                            ngx_updown_status + CACHE_LINE * (i * 2 + 1),
                            value[i].updown_default
                        );
                    } else {
                        ngx_atomic_int_assign(
                            ngx_updown_status + CACHE_LINE * (i * 2 + 1),
                            updown_status
                        );
                    }
                }
            } else {
                ngx_atomic_int_assign(
                    ngx_updown_status + CACHE_LINE * (i * 2 + 1),
                    updown_status
                );
            }
            ngx_atomic_int_assign(
                ngx_updown_status + CACHE_LINE * (i * 2 + 2),
                ngx_murmur_hash2(value[i].name.data, value[i].name.len)
            );
        }
    } else {
        newshm.size = CACHE_LINE * (ulcfs->nelts * 2 + 1);
        newshm.name.len = sizeof("nginx_shared_zone_updown") - 1;
        newshm.name.data = (u_char *) "nginx_shared_zone_updown";
        newshm.exists = 0;
        newshm.log = cycle->log;
        if (ngx_shm_alloc(&newshm) != NGX_OK) {
          return NGX_ERROR;
        }

        ngx_updown_status = newshm.addr;
        value = ulcfs->elts;

        // record size
        ngx_atomic_int_assign(ngx_updown_status, ulcfs->nelts);
        for (i = 0; i < ulcfs->nelts; i++ ) {
            if (value[i].updown_file.len == 0 || value[i].updown_file.len == NGX_CONF_UNSET_SIZE) {
                ngx_atomic_int_assign(
                    ngx_updown_status + CACHE_LINE * (i * 2 + 1),
                    value[i].updown_default
                );
            } else {
                updown_status = ngx_http_updown_sync_from_file(&value[i], cycle->log);
                if (updown_status == -1) {
                    if (ngx_http_updown_write_file(&(value[i].updown_file), cycle->log,
                        value[i].updown_default) == NGX_FILE_ERROR) {

                        ngx_log_error(NGX_LOG_ERR, cycle->log, 0,
                            "updown: sync file error %V", &value[i].updown_file);
                        return NGX_ERROR;
                    }
                    ngx_atomic_int_assign(
                        ngx_updown_status + CACHE_LINE * (i * 2 + 1),
                        value[i].updown_default
                    );
                } else {
                    ngx_atomic_int_assign(
                        ngx_updown_status + CACHE_LINE * (i * 2 + 1),
                        updown_status
                    );
                }
            }
            ngx_atomic_int_assign(
                (ngx_atomic_int_t *)(ngx_updown_status + CACHE_LINE * (i * 2 + 2)),
                ngx_murmur_hash2(value[i].name.data, value[i].name.len)
            );
        }
    }

    return NGX_OK;
}

static void
ngx_http_updown_exit_process(ngx_cycle_t *cycle) {
    // TODO
}

static ngx_int_t
ngx_http_updown_handler_get (ngx_http_request_t *req) {
    u_char ngx_response_body[1024] = {0};
    ngx_http_updown_loc_conf_t *ulcf;
    ngx_int_t rc;

    ulcf = ngx_http_get_module_loc_conf(req, ngx_http_updown_module);
    if (*(ngx_atomic_int_t *)(ngx_updown_status + CACHE_LINE * (2 * ulcf->index + 1)) == 0) {
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
    ngx_atomic_int_assign(ngx_updown_status + (2 * ulcf->index + 1) * CACHE_LINE, 1);
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
    ngx_atomic_int_assign(ngx_updown_status + CACHE_LINE * (2 * ulcf->index + 1), 0);
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
ngx_http_updown_write_file(ngx_str_t *file, ngx_log_t *log, ngx_int_t status) {
    ngx_fd_t fd;

    // #define NGX_FILE_APPEND          (O_WRONLY|O_APPEND)
    // APPEND flag means write is atomic
    fd = ngx_open_file(file->data, NGX_FILE_APPEND, NGX_FILE_CREATE_OR_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
            ngx_open_file_n " \"%s\" failed", file->data);
        return NGX_FILE_ERROR;
    }
    if (ngx_write_fd(fd, status ? "1" : "0", 1) == NGX_ERROR) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
            ngx_write_fd_n" \"%s\" failed", file->data);
        ngx_close_file(fd);
        return NGX_FILE_ERROR;
    }

    ngx_close_file(fd);
    return NGX_OK;
}

// THIS WILL BE OPENED IN 'ONE' MASTER PROCESS
static ngx_int_t
ngx_http_updown_sync_from_file(ngx_http_updown_loc_conf_t *ulcf, ngx_log_t *log) {
    u_char      recv[1] = {0};
    int         n;
    ngx_fd_t    fd;

    fd = ngx_open_file(ulcf->updown_file.data, NGX_FILE_RDWR, NGX_FILE_CREATE_OR_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
            ngx_open_file_n " \"%s\" failed", ulcf->updown_file.data);
        return NGX_FILE_ERROR;
    }
    if (lseek(fd, 0, SEEK_END) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
            "lseek %s failed", ulcf->updown_file.data);
        return NGX_FILE_ERROR;
    }
    n = ngx_read_fd(fd, recv, 1);
    ngx_close_file(fd);
    if (n == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_errno,
          ngx_read_file_n " \"%V\" failed", ulcf->updown_file);
        return 0;
    }

    // file is empty
    if (n == 0) {
        return NGX_FILE_ERROR;
    }

    return ngx_atoi(recv, 1);
}

static ngx_int_t
ngx_http_updown_sync_to_file(ngx_http_request_t *req,
                             ngx_http_updown_loc_conf_t *ulcf) {
    ngx_fd_t fd;

    if (ulcf->updown_file.len == 0 || ulcf->updown_file.len == NGX_CONF_UNSET_SIZE) {
        ngx_log_error(NGX_LOG_INFO, req->connection->log, 0,
            "updown: unset updown_file, so skip");
        return NGX_OK;
    }

    fd = ngx_open_file(ulcf->updown_file.data, NGX_FILE_APPEND, NGX_FILE_CREATE_OR_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_EMERG, req->connection->log, ngx_errno,
            ngx_open_file_n " \"%s\" failed", ulcf->updown_file.data);
        return NGX_FILE_ERROR;
    }

    if (*(ngx_atomic_int_t *)(ngx_updown_status + (2 * ulcf->index + 1) * CACHE_LINE) == 0) {
        ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
          "updown: sync file %V to 0", &ulcf->updown_file);
        if (ngx_write_fd(fd, "0", 1) == NGX_ERROR) {
          ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
            "updown: sync file error %V", &ulcf->updown_file);
            goto fail;
        }
    } else {
        ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
          "updown: sync file %V to 1", &ulcf->updown_file);
        if (ngx_write_fd(fd, "1", 1) == NGX_ERROR) {
          ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
            "updown: sync file error %V", &ulcf->updown_file);
          goto fail;
        }
    }

    ngx_close_file(fd);
    return NGX_OK;

fail:
    ngx_close_file(fd);
    return NGX_ERROR;
}
