ngx_http_updown_module is a an addon for Nginx to graceful up or down

#Directive
>
Syntax: updown on        
Context: location        
Description: enable updown modules

>
Syntax: up_code xxx      
Default: 200       
Context: location        
Description: make uri return http code xxx if server up

>
Syntax: down_code xxx    
Default: 500       
Context: location       
Description: make uri return http code xxx if server down

#Example
````bash
    location = /healthcheck {
      updown;
      up_code 200;
      down_code 500;
    }
````
