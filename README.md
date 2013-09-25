# ngx_lua_module

## Description

each worker process has it's own lua_State object in ngx_lua_main_conf_t structure\.

## Build Status
[![Build Status](https://travis-ci.org/lwch/ngx_lua_module.png)](https://travis-ci.org/lwch/ngx_lua_module)

## Directives

### lua_code_cache

**syntax:** lua_code_cache on | off

**default:** lua_code_cache on

**context:** main

### lua_init

**syntax:** lua_init &lt;lua-script-str>

**default:** nil

**context:** main

**phase:** init process

When there is a syntax error or any other error with script it will write the log and **still working**\.

### lua_init_by_file

**syntax:** lua_init &lt;path-to-lua-script-file>

**default:** nil

**context:** main

**phase:** init process

When there is a syntax error or any other error it will write the log and **still working**\.

### lua_content

**syntax:** lua_content &lt;lua-script-str>

**default:** nil

**context:** location

**phase:** content

Each script **must** return a string or nil\. When there is a syntax error or any other error with script it will pass to [lua_error](#lua_error) **(high priority)** or [lua_error_by_file](#lua_error_by_file)\.

### lua_content_by_file

**syntax:** lua_content_by_file &lt;path-to-lua-script-file>

**default:** nil

**context:** location

**phase:** content

Each script **must** return a string or nil\. When there is a syntax error or any other error with script it will pass to [lua_error](#lua_error) **(high priority)** or [lua_error_by_file](#lua_error_by_file)\. When [lua_code_cache](#lua_code_cache) is on the script will cache with it's absolute path\.

### lua_error

**syntax:** lua_error &lt;lua-script-str>

**default:** nil

**context:** main

**phase:** on error

When there is a syntax error or any other error with script it will write the log and return nothing for client\.

### lua_error_by_file

**syntax:** lua_error_by_file &lt;path-to-lua-script-file>

**default:** nil

**context:** main

**phase:** on error

When there is a syntax error or any other error it will write the log and return nothing for client\. When [lua_code_cache](#lua_code_cache) is on the script will cache with it's absolute path\.

### lua_exit

**syntax:** lua_exit &lt;lua-script-str>

**default:** nil

**context:** main

**phase:** exit process

### lua_exit_by_file

**syntax:** lua_exit_by_file &lt;path-to-lua-script-file>

**default:** nil

**context:** main

**phase:** exit process

## Nginx API for Lua

### ngx.req.args

**syntax:** ngx.req.args

**context:** lua_content\*, lua_error\*

Table for request arguments\.

### ngx.err.msg

**syntax:** ngx.err.msg

**context:** lua_error\*

Error message\.

### ngx.err.stat

**syntax:** ngx.err.stat

**context:** lua_error\*

Signed with error number with process always 404 or 500\.

### ngx.log

**syntax:** ngx.log(log_level, ...)

**context:** lua_content\*, lua_error\*

The log_level from [log_level](https://github.com/nginx/nginx/blob/master/src/core/ngx_log.h)\.

### ngx.var.VARIABLE

**syntax:** ngx.var.VAR_NAME

**context:** lua_content\*, lua_error\*

VARIABLE for nginx\.

### ngx.scp.path

**syntax:** ngx.scp.path

**context:** lua_init_by_file, lua_content_by_file, lua_error_by_file, lua_exit_by_file

Script realpath for current context\.

### ngx.__req__

**syntax:** ngx.\_\_req\_\_

**context:** lua_content\*, lua_error\*

ngx_http_request_t structure\.

## TODO

