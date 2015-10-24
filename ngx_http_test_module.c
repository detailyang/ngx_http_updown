/*
* @Author: detailyang
* @Date:   2015-10-24 10:36:19
* @Last Modified by:   detailyang
* @Last Modified time: 2015-10-24 12:05:16
*/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *set(ngx_conf_t *, ngx_command_t *, void *);
static ngx_int_t handler(ngx_http_request_t *);

static ngx_command_t test_commands[] = {
  {
    ngx_string("test"),
    NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
    set,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL
  },
  ngx_null_command
};

static ngx_http_module_t test_ctx = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

ngx_module_t ngx_http_test_module = {
  NGX_MODULE_V1,
  &test_ctx,
  test_commands,
  NGX_HTTP_MODULE,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NGX_MODULE_V1_PADDING
};

static char *set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
  ngx_http_core_loc_conf_t *corecf;
  corecf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  corecf->handler = handler;
  return NGX_CONF_OK;
};

static ngx_int_t handler(ngx_http_request_t *req) {
  switch(req->method) {
    case NGX_HTTP_GET:
      return get(req);
    // case NGX_HTTP_POST:
    //   return post(req);
    // case NGX_HTTP_DELETE:
    //   return delete(req);
    default:
      return get(req);
  }

  return get(req);
}

//0 is down, 1 is up
static int ngx_updown_status = 0;

static ngx_init_t get(ngx_http_request_t *req) {
  u_char ngx_response_body[1024] = {0};

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "receive request");

  ngx_log_error(NGX_LOG_ERR, req->connection->log, 0,
                          "url:%V method:%V",
                          &req->uri, &req->method_name);

  if (ngx_updown_status == 0) {
    ngx_sprintf(ngx_response_body, "down");
    req->headers_out.status = 500;
  } else {
    ngx_sprintf(ngx_response_body, "up");
    req->headers_out.status = 200;
  }
  req->headers_out.content_length_n = ngx_strlen(ngx_response_body);;
  ngx_str_set(&req->headers_out.content_type, "text/html");
  ngx_http_send_header(req);

  ngx_buf_t *b; b = ngx_pcalloc(req->pool, sizeof(ngx_buf_t));
  ngx_chain_t out;
  out.buf = b;
  out.next = NULL;
  b->pos = html;
  b->last = html + len;
  b->memory = 1;
  b->last_buf = 1;

  return ngx_http_output_filter(req, &out);
}

static ngx_init_t post(ngx_http_request_t *req) {

}

static ngx_init_t delete(ngx_http_request_t *req) {

}