#ifndef _NGX_LUA_MODULE_UTIL_H_
#define _NGX_LUA_MODULE_UTIL_H_

struct u_char;
struct lua_State;
struct ngx_http_request_t;
struct ngx_pool_t;

// initialize lua_State
extern void ngx_lua_module_init(struct lua_State* lua);

// urldecode
extern size_t ngx_lua_module_unescape_uri(u_char* dst, u_char* src, size_t size);

// get or register ngx.req in lua_State
// after call ngx.req in stack top and ngx in second
extern void ngx_lua_module_get_req(struct lua_State* lua);

// get or register ngx.err in lua_State
// after call ngx.err in stack top and ngx in second
extern void ngx_lua_module_get_err(struct lua_State* lua);

// parse args into ngx.req.args with table
extern void ngx_lua_module_parse_args(ngx_pool_t* pool, u_char* buf, size_t size, struct lua_State* lua);

// write error content into ngx.err.msg when error message in stack top
extern void ngx_lua_module_write_error(struct lua_State* lua, ngx_uint_t status);

extern void ngx_lua_module_set_req_obj(struct lua_State* lua, ngx_http_request_t* r);
extern ngx_http_request_t* ngx_lua_module_get_req_obj(struct lua_State* lua);

#endif

