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


#How-To-Work
-------------
ngx_http_updown let the nginx location become a health check interface, including online and offline nginx.

For example:

```bash
location /hc {
  updown;
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
  POST /hc, return 200
  
5. now check the nginx 'status'
  GET /hc, return 200
```

Requirements
------------

ngx_http_updown requires the following to run:

 * [nginx](http://nginx.org/) or other forked version like [openresty](http://openresty.org/)、[tengine](http://tengine.taobao.org/)


Direction
------------
* updown

```
Syntax:	updown;     
Default:	—
Context:	location

location /hc {
  updown;
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
Syntax:	 down_code number;       
Default:	500
Context:	location

location /hc {
  updown;
  down_code 500;
}
```

Production
----------


Contributing
------------

To contribute to updown, clone this repo locally and commit your code on a separate branch.


Author
------

> GitHub [@detailyang](https://github.com/detailyang)     


License
-------
ngx_http_updown is licensed under the [MIT] license.  
