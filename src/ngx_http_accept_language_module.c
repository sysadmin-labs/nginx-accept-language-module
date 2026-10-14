/*
 * Copyright (C) 2008 Guillaume Maury
 * Copyright (c) 2026, Jochen Neumeister <joneum@FreeBSD.org>
 *
 * BSD 2-Clause, see LICENSE.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


/* q travels in thousandths: "q=0.95" is 950, an absent q is 1000.  Three
 * decimals is what the grammar allows, so this is exact rather than a
 * rounding of a float.
 */
#define NGX_HTTP_ACCEPT_LANGUAGE_Q_DEFAULT  1000


static ngx_table_elt_t *ngx_http_accept_language_header(ngx_http_request_t *r);
static ngx_uint_t ngx_http_accept_language_q(u_char **pp, u_char *end);
static ngx_uint_t ngx_http_accept_language_match(u_char *tag, size_t len,
    ngx_str_t *lang);
static char *ngx_http_accept_language(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static ngx_int_t ngx_http_accept_language_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_command_t  ngx_http_accept_language_commands[] = {

    { ngx_string("set_from_accept_language"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_2MORE,
      ngx_http_accept_language,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_accept_language_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */
    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_accept_language_module = {
    NGX_MODULE_V1,
    &ngx_http_accept_language_module_ctx,  /* module context */
    ngx_http_accept_language_commands,     /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


/* The header is looked up in the list rather than read from
 * r->headers_in.accept_language, which exists only in an nginx compiled
 * with NGX_HTTP_HEADERS.  The module this was taken over from reached for
 * that member, and its own config defined the macro to make it appear --
 * which works for a module built into the binary and not for a loadable
 * one, where the structure's shape was decided when that binary was
 * compiled.  Walking the list needs nothing from the build and costs a
 * handful of comparisons once per request.
 */
static ngx_table_elt_t *
ngx_http_accept_language_header(ngx_http_request_t *r)
{
    ngx_list_part_t  *part;
    ngx_table_elt_t  *h;
    ngx_uint_t        i;

    static ngx_str_t  name = ngx_string("Accept-Language");

    part = &r->headers_in.headers.part;
    h = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        if (h[i].key.len == name.len
            && ngx_strncasecmp(h[i].key.data, name.data, name.len) == 0)
        {
            return &h[i];
        }
    }

    return NULL;
}


/* "q=" has been eaten; what follows is "0", "1" or either with up to three
 * decimals.  A value this does not understand leaves the weight at its
 * default: a client that writes nonsense into one parameter still said
 * which languages it wants, and dropping its only candidate over that
 * would answer in the wrong language rather than in none.
 */
static ngx_uint_t
ngx_http_accept_language_q(u_char **pp, u_char *end)
{
    u_char      *p = *pp;
    ngx_uint_t   q, mult, digits;

    if (p >= end || (*p != '0' && *p != '1')) {
        return NGX_HTTP_ACCEPT_LANGUAGE_Q_DEFAULT;
    }

    q = (ngx_uint_t) (*p - '0') * 1000;
    p++;

    if (p < end && *p == '.') {
        p++;

        for (mult = 100, digits = 0;
             p < end && digits < 3 && *p >= '0' && *p <= '9';
             p++, digits++, mult /= 10)
        {
            q += (ngx_uint_t) (*p - '0') * mult;
        }
    }

    *pp = p;

    return (q > 1000) ? 1000 : q;
}


/* A configured "de" is meant to cover "de" and "de-AT", and not "denmark":
 * the match ends at the end of the tag or at a hyphen, which is where a
 * language tag's next subtag begins.
 */
static ngx_uint_t
ngx_http_accept_language_match(u_char *tag, size_t len, ngx_str_t *lang)
{
    if (len < lang->len || lang->len == 0) {
        return 0;
    }

    if (ngx_strncasecmp(tag, lang->data, lang->len) != 0) {
        return 0;
    }

    return (len == lang->len || tag[lang->len] == '-');
}


static char *
ngx_http_accept_language(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t            *value, *elt, name;
    ngx_array_t          *langs;
    ngx_http_variable_t  *var;
    ngx_uint_t            i;

    value = cf->args->elts;
    name = value[1];

    if (name.len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "the variable name must not be empty");
        return NGX_CONF_ERROR;
    }

    if (name.data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "\"%V\" variable name should start with '$'",
                           &name);
    } else {
        name.len--;
        name.data++;
    }

    var = ngx_http_add_variable(cf, &name, NGX_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return NGX_CONF_ERROR;
    }

    if (var->get_handler != NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "variable already defined: \"%V\"", &name);
        return NGX_CONF_ERROR;
    }

    langs = ngx_array_create(cf->pool, cf->args->nelts - 2, sizeof(ngx_str_t));
    if (langs == NULL) {
        return NGX_CONF_ERROR;
    }

    for (i = 2; i < cf->args->nelts; i++) {
        elt = ngx_array_push(langs);
        if (elt == NULL) {
            return NGX_CONF_ERROR;
        }

        *elt = value[i];
    }

    var->get_handler = ngx_http_accept_language_variable;
    var->data = (uintptr_t) langs;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_accept_language_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_array_t      *langs_array = (ngx_array_t *) data;
    ngx_str_t        *langs = langs_array->elts;
    ngx_table_elt_t  *header;
    u_char           *p, *end, *tag;
    size_t            len;
    ngx_uint_t        i, q, best, best_q;

    /* The first configured language is what the server prefers, and that is
     * the answer when the request says nothing this list can serve: no
     * header at all, nothing acceptable, or nothing that matches.
     */
    best = 0;
    best_q = 0;

    header = ngx_http_accept_language_header(r);

    if (header == NULL) {
        goto done;
    }

    p = header->value.data;
    end = p + header->value.len;

    while (p < end) {

        while (p < end && (*p == ' ' || *p == '\t' || *p == ',')) {
            p++;
        }

        if (p == end) {
            break;
        }

        tag = p;

        while (p < end && *p != ',' && *p != ';' && *p != ' ' && *p != '\t') {
            p++;
        }

        len = (size_t) (p - tag);
        q = NGX_HTTP_ACCEPT_LANGUAGE_Q_DEFAULT;

        /* Everything up to the next comma belongs to this candidate.  Only
         * "q" is read; any other parameter is skipped over rather than
         * refused, which is what a receiver is asked to do with a
         * parameter it does not know.
         */
        while (p < end && *p != ',') {

            if (*p == ';') {
                p++;

                while (p < end && (*p == ' ' || *p == '\t')) {
                    p++;
                }

                if ((size_t) (end - p) >= 2
                    && (*p == 'q' || *p == 'Q') && p[1] == '=')
                {
                    p += 2;
                    q = ngx_http_accept_language_q(&p, end);
                    continue;
                }

                continue;
            }

            p++;
        }

        /* q=0 says the client does not want it at all. */
        if (q == 0 || len == 0) {
            continue;
        }

        for (i = 0; i < langs_array->nelts; i++) {

            /* "*" stands for whatever is left, so it speaks for the first
             * language on the list -- the server's own preference.
             */
            if ((len == 1 && tag[0] == '*' && i == 0)
                || ngx_http_accept_language_match(tag, len, &langs[i]))
            {
                /* A higher weight wins.  On the same weight the list
                 * decides, because the order of the header does not: the
                 * client expressed its preference in q, and where it did
                 * not express one, the server's is the next best thing.
                 */
                if (q > best_q || (q == best_q && i < best)) {
                    best = i;
                    best_q = q;
                }

                break;
            }
        }
    }

done:

    if (langs_array->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = langs[best].data;
    v->len = langs[best].len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}
