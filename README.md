# nginx http updown module
![Branch master](https://img.shields.io/badge/branch-master-brightgreen.svg?style=flat-square)[![Build](https://api.travis-ci.org/detailyang/ngx_http_updown.svg)](https://travis-ci.org/detailyang/ngx_http_updown)[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/detailyang/ngx_http_updown/master/LICENSE)[![release](https://img.shields.io/github/release/detailyang/ngx_http_updown.svg)](https://github.com/detailyang/ngx_http_updown/releases)

ngx_http_updown_module is a an addon for Nginx to graceful up or down
Table of Contents
-----------------
* [How-To-Work](#how-to-work)
* [Requirements](#requirements)
* [Direction](#direction)
* [Production](#production)
* [Contributing](#contributing)
* [Author](#author)
* [License](#license)


How-To-Work
----------------

ngx_http_updown let the nginx location become a health check interface, including online and offline nginx.
For example:

```bash
location /hc {
  updown default;
  updown_code 200;
  down_code 500;
  updown_file /data/updown/default.updown; # persistence
  updown_upstream default; # check upstream status
}
```

There are two factors to determine /hc return up or down

1. updown status: control by user
2. upstream status control by upstream_check_status


There are two scene to use updown

1. want to offline the nginx
2. auto offline the nginx if upstream is down

default result as following:

```bash
total:up updown:up upstream:up
```

let's begin to enum what will happen on nginx runtime.

if upstream is down, it will return down whatever updown status as following:

```bash
total:down updown:up upstream:down
total:down updown:down upstream:down
```

if upstream is up, it will return up when updown is up only as following:

```bash
total:up updown:up upstream:up
total:down updown:down upstream:up
```

So the common op is offline and online nginx, it's easy just send DELETE and POST method.

```bash
curl -X DELETE http://127.0.0.1/hc
curl -X GET http://127.0.0.1/hc
total:down updown:down upstream:up

curl -X POST http://127.0.0.1/hc
curl -X GET http://127.0.0.1/hc
total:up updown:up upstream:up
```

Requirements
------------

ngx_http_updown requires the following to run:

 * [nginx](http://nginx.org/) or other forked version like [openresty](http://openresty.org/)、[tengine](http://tengine.taobao.org/)
 * [nginx_upstream_check_module](https://github.com/detailyang/nginx_upstream_check_module)
 make sure have patched by [@detailyang](https://github.com/detailyang) which expose api to check upstream status or if you are using tengine,
make sure you use the latest version which tengine have merged [@detailyang](https://github.com/detailyang) patch to expose api.


Direction
------------
* updown

```
Syntax:	updown string;
Default:	—
Context:	location

location /hc {
  updown default;
}
```

* up_code

```
Syntax:	 up_code number;
Default:	500
Context:	location

location /hc {
  updown;
  up_code 500;
}
```

* down_code

```
Syntax:     down_code number;
Default:	500
Context:	location

location /hc {
  updown;
  down_code 500;
}
```

* updown_file

```
Syntax:     updown_file filename;
Default:    -
Context:    location

location /hc {
  updown default;
  updown_file /data/updown/default.updown;
}

```

* updown_upstream

```
Syntax:     updown_upstream upstream_name;
Default:    -
Context:    location

location /hc {
  updown default;
  updown_upstream default;
}

```

Production
----------
Production deploy as follow:

![production](https://rawgit.com/detailyang/ngx_http_updown/master/docs/deploy.jpg)

As we can see, for the fronted double nginx, we set as a 7 layer load balancer. the backend four nginx, we set as a proxy server to proxy_pass other nodes.

To enable health check nginx node, we should make sure the fronted double nginx set the upstream as follow:

```
upstream app_xx {
    keepalive 64;
    server xx:80  max_fails=0 weight=100;
    server xx:80 max_fails=0 weight=100;
    check interval=1000 rise=5 fall=5 timeout=1000 type=http default_down=false;
    check_http_send "GET /hc HTTP/1.0\r\nConnection: keep-alive\r\n\r\n";
    check_http_expect_alive http_2xx;
}
```

and enable the the updown module on the backend nginx as follow:

```
upstream local_foo {
    keepalive 64;
    server 127.0.0.1:8000  max_fails=0 weight=100;
    check interval=1000 rise=5 fall=5 timeout=1000 type=http default_down=false;
    check_http_send "GET /hc HTTP/1.0\r\nConnection: keep-alive\r\n\r\n";
    check_http_expect_alive http_2xx ;
}

upstream local_bar {
    keepalive 64;
    server 127.0.0.1:9000 max_fails=0 weight=100;
    check interval=1000 rise=5 fall=5 timeout=1000 type=http default_down=false;
    check_http_send "GET /hc HTTP/1.0\r\nConnection: keep-alive\r\n\r\n";
    check_http_expect_alive http_2xx ;
}

server {
    listen       80 default;

    charset utf-8;
    
    location = /hc {
        updown default;
        down_code 404;
        updown_file /data/updown/default.updown;
        updown_upstream local_foo;
    }

    location = /anotherhc {
        updown another;
        down_code 404;
        updown_file /data/updown/another.updown;
        updown_upstream local_bar;
    }
}
```

Contributing
------------

To contribute to ngx_http_updown, clone this repo locally and commit your code on a separate branch.


Author
------

> GitHub [@detailyang](https://github.com/detailyang)


License
-------
ngx_http_updown is licensed under the [MIT] license.

[MIT]: https://github.com/detailyang/ybw/blob/master/licenses/MIT
