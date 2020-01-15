# Download-Server
server that allows anonymous downloads.


How To Use This Program.

#### Step 1 Compile server.cpp and the client.cpp files and create different executables.
```bash
clang++ -std=c++11 client.cpp -o client

clang++ -std=c++11 server.cpp -o server

```
#### Step 2 Run The server first by the command:  

```bash
./server <port number>
Note: port number must be between 1025 & 65535
```

#### Step 3 Run The Client by the command: 

```bash
./client <Host Name or Ip Address of Server> <port number of server>
```

# Example 

## Server Side
### This will start the serrver side program and will open the port to listent to incoming connections.
```bash
clang++ -std=c++11 server.cpp -o server

./server 5556
```


### Client Side:
### This will start the client side program and will open a connection to the port provided to the server.
```bash
clang++ -std=c++11 client.cpp -o client

./client 127.0.0.1 5556 
````

### After this a connection will be established between client and server and you will get a navigation menu that looks like this:
       
                  ******************************************************************************
                  *                               Menu Options:                                *
                  *                                                                            *
                  *       PWD - Prints working directory on server                             *
                  *       DIR - Prints each file name in current directory on server           *
                  *       CD <Directory Name>  - Changes Directory to directory specified      *
                  *       Download <fileName> - Download specified file                        *
                  *       Bye - Disconnect from server                                         *
                  *                                                                            *
                  ******************************************************************************

#### You can navigate through directories and download files, when done Type "Bye" to exit program.

# Enjoy :>>
