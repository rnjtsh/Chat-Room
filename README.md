# Chat-Room
The repository contains a chat room application based on client-server communication.
Features:
1. Multiple active users can be connected to the server, but the maximum number of users needs to be mentioned while starting the server.
2. Multiple chat rooms can be run in parallel.
3. Users can list the active chatrooms, choose to join one of them, create a chatrrom, add another user to the same chatoom, send files over the chat.


Instruction to run server:

bash server.sh <client limit> <server ip> <server port>

Instruction to run server:

bash client.sh <client name> <client ip> <client port1> <client port2> <server ip> <server port>
  
Commands:
1.  create chatroom <chatroom name>
2.  join <chatroom nam>
3.  list chatrooms
4.  list users -> Lists all users in the same chatroom
5.  reply "<message>"
6.  leave
7.  reply <filename> <tcp/udp>
  
  
NOTE: File transfer using UDP is not yet implemented.
