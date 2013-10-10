# ngx\_lua\_module

## Description

each worker process has it's own lua\_State object in ngx\_lua\_main\_conf\_t structure\.

## Build Status
[![Build Status](https://travis-ci.org/lwch/ngx_lua_module.png)](https://travis-ci.org/lwch/ngx_lua_module)

## Directives

### lua\_code\_cache

**syntax:** lua\_code\_cache on | off

**default:** lua\_code\_cache on

**context:** main

### lua\_init

**syntax:** lua\_init &lt;lua-script-str>

**default:** nil

**context:** main

**phase:** init process

When there is a syntax error or any other error with script it will write the log and **still working**\.

### lua\_init\_by\_file

**syntax:** lua\_init\_by\_file &lt;path-to-lua-script-file>

**default:** nil

**context:** main

**phase:** init process

When there is a syntax error or any other error it will write the log and **still working**\.

### lua\_content

**syntax:** lua\_content &lt;lua-script-str>

**default:** nil

**context:** location

**phase:** content

Each script **must** return a string or nil\. When there is a syntax error or any other error with script it will pass to [lua_error](#lua_error) **(high priority)** or [lua_error_by_file](#lua_error_by_file)\.

### lua\_content\_by\_file

**syntax:** lua\_content\_by\_file &lt;path-to-lua-script-file>

**default:** nil

**context:** location

**phase:** content

Each script **must** return a string or nil\. When there is a syntax error or any other error with script it will pass to [lua_error](#lua_error) **(high priority)** or [lua_error_by_file](#lua_error_by_file)\. When [lua_code_cache](#lua_code_cache) is on the script will cache with it's absolute path\.

### lua\_access

**syntax:** lua\_access &lt;lua-script-str>

**default:** nil

**context:** location

**phase:** access

Each script **must** return true or false\. When there is a syntax error or any other error with script it will pass to [lua_error](#lua_error) **(high priority)** or [lua_error_by_file](#lua_error_by_file)\.

### lua\_access\_by\_file

**syntax:** lua\_access\_by\_file &lt;path-to-lua-script-file>

**default:** nil

**context:** location

**phase:** access

Each script **must** return true or false\. When there is a syntax error or any other error with script it will pass to [lua_error](#lua_error) **(high priority)** or [lua_error_by_file](#lua_error_by_file)\. When [lua_code_cache](#lua_code_cache) is on the script will cache with it's absolute path\.

### lua\_error

**syntax:** lua\_error &lt;lua-script-str>

**default:** nil

**context:** main

**phase:** on error

When there is a syntax error or any other error with script it will write the log and return nothing for client\.

### lua\_error\_by\_file

**syntax:** lua\_error\_by\_file &lt;path-to-lua-script-file>

**default:** nil

**context:** main

**phase:** on error

When there is a syntax error or any other error it will write the log and return nothing for client\. When [lua_code_cache](#lua_code_cache) is on the script will cache with it's absolute path\.

### lua\_exit

**syntax:** lua\_exit &lt;lua-script-str>

**default:** nil

**context:** main

**phase:** exit process

### lua\_exit\_by\_file

**syntax:** lua\_exit\_by\_file &lt;path-to-lua-script-file>

**default:** nil

**context:** main

**phase:** exit process

## Nginx API for Lua

### ngx.req.args

**syntax:** ngx.req.args

**context:** lua\_content\*, lua\_access\*, lua\_error\*

Table for request arguments\.

### ngx.err.msg

**syntax:** ngx.err.msg

**context:** lua\_error\*

Error message\.

### ngx.err.stat

**syntax:** ngx.err.stat

**context:** lua\_error\*

Signed with error number with process always 404 or 500\.

### ngx.log

**syntax:** ngx.log(log\_level, ...)

**context:** lua\_content\*, lua\_access\*, lua\_error\*

The log\_level from [log_level](https://github.com/nginx/nginx/blob/master/src/core/ngx_log.h)\.

### ngx.var.VARIABLE

**syntax:** ngx.var.VAR\_NAME

**context:** lua\_content\*, lua\_access\*, lua\_error\*

VARIABLE for nginx\.

### ngx.scp.path

**syntax:** ngx.scp.path

**context:** lua\_init\_by\_file, lua\_content\_by\_file, lua\_access\_by\_file, lua\_error\_by\_file, lua\_exit\_by\_file

Script realpath for current context\.

### ngx.\_\_req\_\_

**syntax:** ngx.\_\_req\_\_

**context:** lua\_content\*, lua\_access\* lua\_error\*

ngx\_http\_request\_t structure\.

## TODO

