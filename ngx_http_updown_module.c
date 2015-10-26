/*
* @Author: detailyang
* @Date:   2015-10-24 10:36:19
* @Last Modified by:   detailyang
* @Last Modified time: 2015-10-26 15:45:48
*/
#include "ngx_http_updown_module.h"

static char *ngx_http_updown_code_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_updown_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_updown_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_updown_handler(ngx_http_request_t *req);
static ngx_int_t ngx_http_updown_module_init(ngx_cycle_t *cycle);

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
    ngx_http_updown_code_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_updown_loc_conf_t, up_code),
    NULL },
  {
    ngx_string("down_code"),
    NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
    ngx_http_updown_code_set,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_updown_loc_conf_t, down_code),
    NULL },
  ngx_null_command
};

static ngx_http_module_t ngx_http_updown_module_ctx = {
  NULL,                          /* preconfiguration */
  NULL,          /* postconfiguration */

  NULL,                          /* create main configuration */
  NULL,                          /* init main configuration */

  NULL,                          /* create server configuration */
  NULL,                          /* merge server configuration */

  ngx_http_updown_create_loc_conf, /* create location configuration */
  NULL                        /* merge location configuration */
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
  NULL,                          /* exit process */
  NULL,                          /* exit master */
  NGX_MODULE_V1_PADDING
};

static void *ngx_http_updown_create_loc_conf(ngx_conf_t *cf) {
  ngx_http_updown_loc_conf_t *local_conf = NULL;
  local_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_updown_loc_conf_t));
  if (local_conf == NULL)
  {
          return NULL;
  }

  local_conf->up_code = NGX_CONF_UNSET;
  local_conf->down_code = NGX_CONF_UNSET;

  return local_conf;
}

static char *ngx_http_updown_code_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
  ngx_http_updown_loc_conf_t *local_conf = NULL;
  local_conf = conf;
  char* rv = NULL;

  rv = ngx_conf_set_num_slot(cf, cmd, conf);

  return rv;
}

static char *ngx_http_updown_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
  ngx_http_core_loc_conf_t  *clcf;

  clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_updown_handler;

  return NGX_CONF_OK;
};


//0 is down, 1 is up
static ngx_atomic_t ngx_updown_status0 = 1;
ngx_atomic_t         *ngx_updown_status = &ngx_updown_status0;

static ngx_int_t
ngx_http_updown_module_init(ngx_cycle_t *cycle) {
  u_char              *shared;
  size_t               size, cl;
  ngx_shm_t            shm;

  cl = 128;
  size = cl; /* updown status*/

  shm.size = size;
  shm.name.len = sizeof("nginx_shared_zone_updown");
  shm.name.data = (u_char *) "nginx_shared_zone_updown";
  shm.log = cycle->log;

  if (ngx_shm_alloc(&shm) != NGX_OK) {
    return NGX_ERROR;
  }

  shared = shm.addr;

  ngx_updown_status = (ngx_atomic_t *) (shared);
  (void) ngx_atomic_cmp_set(ngx_updown_status, 0, 1);

  return NGX_OK;
}

static ngx_int_t ngx_http_updown_handler_get (ngx_http_request_t *req) {
  u_char ngx_response_body[1024] = {0};
  ngx_http_updown_loc_conf_t *conf;
  ngx_int_t rc;

  conf = ngx_http_get_module_loc_conf(req, ngx_http_updown_module);

  if (*ngx_updown_status == (ngx_atomic_t) 0) {
    ngx_sprintf(ngx_response_body, "down");
    req->headers_out.status = (conf->down_code == NGX_CONF_UNSET ? DEFAULT_DOWN_CODE: conf->down_code);
  } else {
    ngx_sprintf(ngx_response_body, "up");
    req->headers_out.status = (conf->up_code == NGX_CONF_UNSET ? DEFAULT_UP_CODE : conf->up_code);
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

static ngx_int_t ngx_http_updown_handler_post(ngx_http_request_t *req) {
  u_char ngx_response_body[1024] = {0};
  ngx_http_updown_loc_conf_t *conf;
  ngx_int_t rc;

  conf= ngx_http_get_module_loc_conf(req, ngx_http_updown_module);
  (void) ngx_atomic_cmp_set(ngx_updown_status, 0, 1);
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

static ngx_int_t ngx_http_updown_handler_delete(ngx_http_request_t *req) {
  u_char ngx_response_body[1024] = {0};
  ngx_http_updown_loc_conf_t *conf;
  ngx_int_t rc;

  conf= ngx_http_get_module_loc_conf(req, ngx_http_updown_module);
  (void) ngx_atomic_cmp_set(ngx_updown_status, 1, 0);
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

static ngx_int_t ngx_http_updown_handler(ngx_http_request_t *req) {
  switch(req->method) {
    case NGX_HTTP_GET:
      return ngx_http_updown_handler_get(req);
    case NGX_HTTP_POST:
      return ngx_http_updown_handler_post(req);
    case NGX_HTTP_DELETE:
      return ngx_http_updown_handler_delete(req);
    default:
      return ngx_http_updown_handler_get(req);
  }

  return ngx_http_updown_handler_get(req);
}
