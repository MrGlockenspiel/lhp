# lhp
The Lua Hypertext Preprocessor.

`lhp` is a minimal PHP-like FastCGI server written in C with Lua 5.1/LuaJIT.
It allows embedding inline Lua code in `.lhp` template files.

## Features
 - Execute Lua code in HTML templates: `<?lua print("<p>Hello world</p>") ?>`
 - `require()` modules in the script root for modular and extensible code
 - Server utilizes FastCGI for integration with web servers like Nginx and lighttpd

## Building
Clone the repository and run
```sh
make
```
`lhp` is the compiled server binary.

### Dependencies
Depends on:
 - `libfcgi-dev`
 - `liblua5.1-0-dev` or `libluajit-5.1-dev`
 - `make`
 - A good C compiler, i.e., `gcc` or `clang`


## Running
Start server with `spawn-fcgi` on the deafult port of 9000:
```sh
make run
```

You'll still need a web server to handle the requests.

Example Nginx config:
```nginx
server {
    listen 80;
    root /path/to/www;

    index index.lhp;
    
    error_page 404 /404.lhp;

    location ~ \.lhp$ {
        include fastcgi_params;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        fastcgi_pass 127.0.0.1:9000;
        fastcgi_intercept_errors on;
    }

    error_page 404 /404.lhp;
}
```

Example `lhp` files can be found in the `www/` directory