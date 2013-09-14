# ngx_lua_module

## Directives

### lua_code_cache

**syntax:** lua_code_cache on | off

**default:** lua_code_cache on

**context:** main, server, location

### lua_init

**syntax:** lua_init &lt;lua-script-str>

**default:** nil

**context:** server

**phase:** init process

When there is a syntax error or any other error with script it will write the log and **still working**\.

### lua_init_by_file

**syntax:** lua_init &lt;path-to-lua-script-file>

**default:** nil

**context:** server

**phase:** init process

When there is a syntax error or any other error it will write the log and **still working**\.

### lua_content

**syntax:** lua_content &lt;lua-script-str>

**default:** nil

**context:** location

**phase:** content

Each script **must** return a string or nil\. When there is a syntax error or any other error with script it will pass to [lua_error](#7)**(high priority)** or [lua_error_by_file](#8)\.

### lua_content_by_file

**syntax:** lua_content_by_file &lt;path-to-lua-script-file>

**default:** nil

**context:** location

**phase:** content

Each script **must** return a string or nil\. When there is a syntax error or any other error with script it will pass to [lua_error](#7)**(high priority)** or [lua_error_by_file](#8)\. When there is a file not found error it will return 404 error\. When [lua_code_cache](#2) is on the script will cache with it's absolute path\.

### lua_error

**syntax:** lua_error &lt;lua-script-str>

**default:** nil

**context:** server

**phase:** on error

When there is a syntax error or any other error with script it will write the log and return nothing for client\.

### lua_error_by_file

**syntax:** lua_error_by_file &lt;path-to-lua-script-file>

**default:** nil

**context:** server

**phase:** content

When there is a syntax error or any other error it will write the log and return nothing for client\.

## Nginx API for Lua

### ngx.req.args

**syntax:** args = ngx.req.args

**context:** lua_content\*, lua_error\*

table for request

### ngx.log

**syntax:** ngx.log(log_level, ...)

**context:** lua_content\*, lua_error\*

## TODO

