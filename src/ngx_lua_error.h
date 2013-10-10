#ifndef _NGX_LUA_ERROR_H_
#define _NGX_LUA_ERROR_H_

extern ngx_int_t ngx_lua_content_call_error(ngx_http_request_t* r, struct lua_State* lua, ngx_uint_t status);

#endif

