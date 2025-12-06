## config file rules:

Allow multiple servers<br/>
Each server must have location<br/>
port, app with version, directory, routers as array<br/><br/>

## example

{
  "servers": [
    {
      "port": 8080,
      "name": "3xx",
      "version": "0.1.0",
      "root": "./app/my-app",
      "routes": [
        { "path": "/", "source": "./index.html", "method": ["GET"] },
        { "path": "/about", "source": "./about.html", "method": ["GET"] }
      ]
    }
  ]
}