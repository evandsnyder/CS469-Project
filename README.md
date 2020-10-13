# CS469 Project

Distributed system for project CS469 \
Andrew Egeler \
Evan Snyder

We've chosen prompt 2 for our distributed systems assignment

## Installation
There are several tools included to get the application up and running. Follow along to install and configure the tool.

##### Required tools and Packages
* python3
* CMake
* libgtk-3-dev
* sqlite3
* libssl-dev

The first thing to do is to create a simple database to be used. A python script as been included to facilitate this process:
```
python3 create_db.py  
```
This will produce an `items.db` file that can be used for the item database tool.

At this point, we can build all of the tools using CMake:
```
cmake .
make
```
We still need to make a user account for login purposes. A tool has also been provided for this purpose:
```
./user_mgr username password
```
This will store the username and encrypted password into the database.

Finally, a server key and certificate must be available to encrypt the network traffic. This can be done via:
```
openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out cert.pem
```

**At this point, all applications are ready to be run.**

### Configurations

The server application takes command line arguments as well as a config file to prepare the server. 
Command line arguments may appear as:
```
./server -l 4466 -s localhost -p 6644 -k key.pem -c server.conf -d items.db -i 1:H
```

A Sample config file may look like:
```
PORT=4466
BACKUP_SERVER=localhost
BACKUP_PORT=6644
DATABASE=items.db
INTERVAL=24:m
```


#### TODO:
* ~~Client Login UI~~
* ~~Client Main UI~~
* ~~Client Connect to server~~
* ~~Server argument parsing~~
* ~~Server SSL/TLS Listening~~
* ~~Server Multithreaded Client Accept~~
* ~~Server database operations thread~~
  * ~~Open Database~~
  * ~~Read from request queues~~
  * ~~Do Operation~~
  * ~~Send reponse back to requesting thread~~
  * ~~Periodic backup to datastore server~~
  * ~~DB Shutdown and closing~~
 * ~~Server Threadsafe queue~~
 * ~~Timer Thread~~
 * ~~Datastore SSL/TLS Listen~~
 * ~~Datastore SSL download and store file~~
 * ~~Datastore config file~~
