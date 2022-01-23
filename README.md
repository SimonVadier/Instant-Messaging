# Instant_Messaging
Instant messaging project
## Introduction
This project aims at setting up an instant messaging between clients connected on a server.
These clients, once connected, must first register a pseudonym and will be assigned an ID.
This ID allows the different clients to add each other as friends through a menu and then to start a private chat room.
## Setup
First you need to compile the modules, in /module: ```make```

Then you have to compile the project, in /project: ```make```

The project is compiled. Now you just have to launch the server and the different clients (maximum 50).

For the server:
```./Projet/server <PORT>``` 

For the client:
```./Projet/client <IP><PORT>```

## Functionalities
### For the server
- Creation of a thread pool waiting for connections
- Accept connections 
- Add a friend:
	- Write the list of connected people
	- Write friend add in a database
- Start a conversation:
	- Display the list of connected friends
	
### For the clients

- Display a menu with:
	- Add a friend
	- Start a conversation
	- Quit
- Allow sending and receiving a message.

