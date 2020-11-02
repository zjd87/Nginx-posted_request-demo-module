/*
 * Author: Zhang Jinde
 * E-mail: zjd5536@163.com
 */

#include <ngx_core.h>
#include <ngx_config.h>
#include <ngx_http.h>

typedef struct {
    ngx_str_t stock[6];
}ngx_http_mytest_ctx_t;

static void
mytest_post_handler(ngx_http_request_t *r)
{

}

static ngx_int_t
mytest_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_http_request_t   *pr = r->parent;

    ngx_http_mytest_ctx_t  *myctx = ngx_http_get_module(pr, ngx_http_mytest_module);
    pr->headers_out.status = r->headers_out.status;

    if (NGX_HTTP_OK == r->headers_out.status) {
        int flag = 0;
        ngx_buf_t *pRecvBuf = &r->upstream->buffer;

        for (;pRecvBuf->pos != pRecvBuf->last; pRecvBuf->pos++ ) {
            if (*pRecvBuf->pos == ',' || *pRecvBuf->pos == '\"') {
              if (flag > 0) {
                myctx->stock[flag-1].len = pRecvBuf->pos - myctx->stock[flag-1].data;
              }
              flag++;
              myctx->stock[flag-1].data = pRecvBuf->pos + 1;
            }
            if (flag > 6) {

              break;
            }
        }
    }

    pr->write_event_handler = mytest_post_handler;

    return NGX_OK;
}

static ngx_int_t
ngx_http_mytest_handler(ngx_http_request_t *r)
{
    ngx_http_mytest_ctx_t *myctx= ngx_http_get_module_ctx(r, ngx_http_mytest_module);

    if (myctx == NULL) {
        myctx =ngx_palloc(r->pool, sizeof(ngx_http_mytest_ctx_t));
        if (myctx == NULL) {
            return NGX_ERROR;
        }

        ngx_http_set_ctx(r, myctx, ngx_http_mytest_module);
    }

    ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
    if (psr == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    psr->handler = mytest_subrequest_post_handler;
    psr->data = myctx;

    ngx_str_t sub_prefix = ngx_string("/list=");
    ngx_str_t sub_location;
    sub_location.len = sub_prefix.len + r->args.len;
    sub_location.data = ngx_palloc(r->pool, sub_location.len);

    ngx_snprintf(sub_location.data, sub_location.len, "%V%V", &sub_prefix, &r->args);

    ngx_http_request_t *sr;
    ngx_int_t rc = ngx_http_subrequest(r, &sub_location, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_DONE;
}
static char *
ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_mytest_handler;
    return NGX_CONF_OK;
}

static ngx_command_t ngx_http_mytest_commands[] = {

    { ngx_string("mytest"),
      NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
      ngx_http_mytest,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},

    ngx_null_command
};
