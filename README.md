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
}


1. when nginx is 'online'
  GET /hc, return 200

2. now ready to offline nginx
  DELETE /hc, if success return 200

3. now check the nginx 'status'
  GET /hc, return 500

4. now ready to online nginx
  POST /hc, if success return 200

5. now check the nginx 'status'
  GET /hc, return 200
```

Requirements
------------

ngx_http_updown requires the following to run:

 * [nginx](http://nginx.org/) or other forked version like [openresty](http://openresty.org/)、[tengine](http://tengine.taobao.org/)
 * [nginx_upstream_check_module](https://github.com/yaoweibin/nginx_upstream_check_module)


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
  updown_file /data/updown/default.updown
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
    check_http_expect_alive http_2xx ;
}
```

and enable the the updown module on the backend nginx as follow:

```
server {
    listen       80 default backlog=65535 rcvbuf=512k;

    charset utf-8;
    access_log  /data/logs/tengine/default.access.log  main;

    error_page  404              /404.html;
    error_page  500 502 503 504  /50x.html;

    location / {
        root   html;
        index  index.html index.htm;
        allow  127.0.0.1;
        allow  10.0.0.0/8;
        deny   all;
    }

    location = /50x.html {
        root   html;
    }

    location /nginx_status {
             stub_status             on;
             access_log              off;
             allow   127.0.0.1;
             allow  10.0.0.0/8;
             deny    all;
    }

    location = /traffic_status {
        req_status_show;
    }

    location = /hc {
        updown default;
        down_code 404;
        updown_file /data/updown/default.updown;
    }

    location = /anotherhc {
        updown another;
        down_code 404;
        updown_file /data/updown/another.updown;
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
