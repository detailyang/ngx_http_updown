ngx_http_updown_module is a an addon for Nginx to graceful up or down

Table of Contents
-----------------
* [Requirements](#requirements)
* [Direction](#direction)
* [Contributing](#contributing)
* [Author](#author)
* [License](#license)

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

Contributing
------------

To contribute to updown, clone this repo locally and commit your code on a separate branch.


Author
------

> GitHub [@detailyang](https://github.com/detailyang)     


License
-------
ngx_http_updown is licensed under the [MIT] license.  
