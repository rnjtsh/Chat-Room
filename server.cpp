#include <stdlib.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <csignal>
#include <chrono>
#include <ctime>

using namespace std;

#define ERROR -1
// #define MAX_CLIENTS 10
#define MAX_DATA 1024
#define BUFFER 1024
#define FILE_BUFFER 1024

#define HTTIMER 300
#define dummyChatRoom "uninitializedChatRoom"

struct socketInfo {
	int nw;
	struct sockaddr_in cli;
};
struct chatMessageInfo {
	string ip;
	string port;
	string sender;
	string message;
	string flag;
};
int sock, MAX_CLIENTS;
const char *server_ip, *server_port, *broadcast_port;
vector<pthread_t> threadV;

unordered_map<string, set<string>> chatRoomTouser;
unordered_map<string, pair<pair<string, string>, string>> userToIPAndChatRoom;
unordered_set<string> ipInUse;

void signalHandler_KillingThreads( int signum ) {
	std::vector<pthread_t>::iterator thit;
	for (thit = threadV.begin(); thit != threadV.end(); thit++) {
		//std::cout << "Killing thread : " << *thit << '\n';
		pthread_kill(*thit, signum);
	}
	threadV.empty();
	close(sock);
	exit(signum);
}

string createChatRoom(string chatRoomName, string user, string ip, string port) {
	string retSt = "";
	auto tempItr = chatRoomTouser.find(chatRoomName);
	if (tempItr != chatRoomTouser.end())
		retSt = "A Chat Room having this name already exists!";
	else
	{
		set<string> tempSet;
		tempSet.insert(user);

		chatRoomTouser.insert(make_pair(chatRoomName, tempSet));
		auto it = userToIPAndChatRoom.find(user);
		it->second.second = chatRoomName;
		// printMaps();
		retSt = "Chatroom created!";
	}
	return retSt;
}
string listFunctions(string specification, string user, string ip, string port) {
	string temp, retSt = "";
	transform(specification.begin(), specification.end(), std::back_inserter(temp), ::toupper);
	if (temp == "CHATROOMS")
		for (auto it : chatRoomTouser)
			retSt += it.first + "\n";
	else if (temp == "USERS")
	{
		auto itr = userToIPAndChatRoom.find(user);
		string chatRoom = itr->second.second;
		if (chatRoom == dummyChatRoom)
			retSt = "You do not belong to any chat room!";
		else
		{
			auto tempItr = chatRoomTouser.find(chatRoom);
			for (auto it = tempItr->second.begin(); it != tempItr->second.end(); it++)
				retSt += *it;
		}
	}
	return retSt;
}
string leaveChatRoom(string user, string ip, string port) {
	string retSt = "";
	auto itr = userToIPAndChatRoom.find(user);
	if (itr->second.second != dummyChatRoom)
	{
		string existingChatRoom = itr->second.second;
		itr->second.second = dummyChatRoom;
		auto tempItr = chatRoomTouser.find(existingChatRoom);
		tempItr->second.erase(user);
		// printMaps();
		retSt = "Deregistered from the chatroom!";
	}
	else
		retSt = "You do not belong to any chatroom!";
	
	return retSt;
}
string joinChatroom(string chatRoomName, string user, string ip, string port) {
	string retSt = "";
	auto it = chatRoomTouser.find(chatRoomName);
	if (it == chatRoomTouser.end())
		retSt = "No existent chatroom!";
	else
	{
		
		it->second.insert(user);
		auto itr1 = userToIPAndChatRoom.find(user);
		cout << "join else " << itr1->second.second << endl;
		string currentChatRoom = itr1->second.second;
		auto itr2 = chatRoomTouser.find(currentChatRoom);
		if (itr2 != chatRoomTouser.end())
			retSt = "User already in another chatroom!\n";
		
		else
		{
			itr1->second.second = chatRoomName;
			// printMaps();
			retSt = "Joined in chatroom!";
		}
	}
	return retSt;
}
string addUserToChatRoom(string chatRoomName, string userToBeAdded, string ip, string port) {
	string retSt = "";
	auto it = userToIPAndChatRoom.find(userToBeAdded);
	if (it == userToIPAndChatRoom.end())
		retSt = "Invalid user name!";
	else
		if (it->second.second != dummyChatRoom)
			retSt = "Requested user is in another chatroom!";
		else
		{
			it->second.second = chatRoomName;
			auto itr = chatRoomTouser.find(chatRoomName);
			itr->second.insert(userToBeAdded);
			retSt = "User successfully added to the Chatroom!";
			// printMaps();
		}
	return retSt;
}
string addNewUser(string userName, string user, string ip, string port) {
	string retSt = "";
	auto it = userToIPAndChatRoom.find(userName);
	if (it != userToIPAndChatRoom.end() || ipInUse.find(ip) != ipInUse.end())
		retSt = "User name or IP already in use. Please use other user credentials!!\n";
	else
	{
		pair<pair<string, string>, string> p;
		p.first.first = ip;
		ipInUse.insert(ip);
		p.first.second = port;
		p.second = dummyChatRoom;
		userToIPAndChatRoom.insert(make_pair(userName, p));
		retSt = "New user successfully added!!\n";
		// printMaps();
	}
	return retSt;
}
vector<pair<string, string>> replyMessage(string user, string ip, string port) {
	vector<pair<string, string>> membersOfChatRoom;
	auto it = userToIPAndChatRoom.find(user);
	if (it->second.second == dummyChatRoom)
		cout << "User doesn't belong to any chatroom!!\n";
	else
	{
		auto it1 = chatRoomTouser.find(it->second.second);
		// cout << "people in same room are\n";
		for (auto itr = it1->second.begin(); itr != it1->second.end(); itr++)
		{
			if (*itr != user)
			{
				// cout << *itr << "->";
				auto tempit = userToIPAndChatRoom.find(*itr);
				// cout << tempit->second.first.first << " " << tempit->second.first.second << endl;
				membersOfChatRoom.push_back(tempit->second.first);
			}
		}
	}
	return membersOfChatRoom;
}
void receiveFile(int sock, const char *filename){
	FILE *fp = fopen(filename, "w+");
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	long totalByteTransfer = 0;
	size_t bytesRcvd;
	char in[FILE_BUFFER];
	rewind(fp);
	if (fp)
	{
		while (1)
		{
			// cout << "Next Block..." << endl;
			bytesRcvd = recv(sock, in, sizeof(in), 0);
			// cout<< "Recieved " << bytesRcvd << " bytes " << endl;
			if (bytesRcvd < 0)
				//perror("recv");
				std::cout << "FAILURE:FILE RECEIVE ERROR" << '\n'
						  << ">>> ";
			else if (bytesRcvd == 0)
				break;
			if (fwrite(in, 1, bytesRcvd, fp) != (size_t)bytesRcvd)
			{
				//perror("fwrite");
				std::cout << "FAILURE:FILE WRITE ERROR" << '\n';
				fclose(fp);
				return;
			}
			totalByteTransfer += (long)bytesRcvd;
		}
		fclose(fp);
		cout << "SUCCESS: FILE RECEIVED IN SERVER" << '\n';
	}
	else
		std::cout << "FAILURE:FILE OPENING ERROR IN RECEIVER" << '\n';
}
void tcpProtocol()
{
}
void udpProtocol()
{
}

void sendFile(int sock, const char *fileName)
{
	// cout << "**FILE SENDING START** " << fileName << endl;
	string slash = "./";
	FILE *fp = fopen(strcat((char *)slash.c_str(), fileName), "r");

	size_t bytesRead, readSize;
	char buff[FILE_BUFFER];

	if (fp)
	{
		cout << "SENDING FILE TO SERVER..." << endl;
		while (!feof(fp))
		{
			// cout << "Next Block..." << endl;
			bytesRead = fread(buff, 1, FILE_BUFFER, fp);
			// std::cout << (int)bytesRead << " bytes will be sent.." << '\n';
			if (bytesRead <= 0)
				break;

			if (send(sock, buff, bytesRead, 0) != bytesRead)
			{
				perror("File send error");
				break;
			}
		}
		cout << "SUCCESS:FILE SHARED" << '\n';
	}
	else
	{
		std::cout << "FAILURE: FILE NOT FOUND" << '\n';
	}
	fclose(fp);
}

// void * chat_broadcast(void * chatData) {
void chat_broadcast(chatMessageInfo data)
{
	cout << "\n";
	// chatMessageInfo *data = ((chatMessageInfo *)chatData);
	cout << "\n";
	string messageToServer = "";
	int cliSock;
	struct sockaddr_in client;
	memset(&client, '0', sizeof(client));
	client.sin_family = AF_INET;

	// cout << stoi((data->receiver).second) << " " << ((data->receiver).first).c_str() << endl;
	// cout << "chat_broadcast function " << data->ip << endl;

	client.sin_port = htons(stoi(data.port));
	client.sin_addr.s_addr = inet_addr((data.ip).c_str());
	// client.sin_addr.s_addr = inet_addr("127.0.0.3");
	if ((cliSock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
	{
		cout << "SOCKET CREATION ERROR!" << endl;
		// pthread_exit(0);
	}

	if ((connect(cliSock, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == ERROR)
	{
		cout << "CONNECTION FAILURE: USER OFFLINE!" << '\n';
		close(cliSock);
		// pthread_exit(0);
	}
	std::cout << "CONNECTION WITH USER ESTABLISHED..." << '\n';
	string instruction = data.flag;
	send(cliSock, instruction.c_str(), instruction.length(), 0);
	if (data.flag == "chat") {
		messageToServer = data.sender + ":  " + data.message;
		send(cliSock, messageToServer.c_str(), messageToServer.length(), 0);
	}
	else {
		messageToServer = data.message;
		send(cliSock, messageToServer.c_str(), messageToServer.length(), 0);
		sendFile(cliSock, messageToServer.c_str());
	}
	
	close(cliSock);
	// pthread_exit(0);
}

void broadcast(vector<pair<string, string>> clientInfo, string sender, string message, string flag) {
	pthread_t thread;
	cout << "broadcast called " << endl;
	for(auto it : clientInfo) {
		cout << "in for broadcast" << endl;
		struct chatMessageInfo *chatData = (struct chatMessageInfo *)malloc(sizeof(chatMessageInfo));
		cout << "in for broadcast 1 " << it.first <<  " " << it.second << endl;
		chatData->ip = it.first;
		chatData->port = it.second;
		cout << "in for broadcast 2 " << sender << endl;
		chatData->sender = sender;
		cout << "in for broadcast 3 " << message << endl;
		chatData->message = message;
		cout << "in for broadcast 4 " << flag << endl;
		chatData->flag = flag;
		cout << "thread created" << endl;



		chat_broadcast(*chatData);
		// pthread_create(&thread, NULL, &chat_broadcast, (void *)chatData);
		// threadV.push_back(thread);
		free(chatData);
	}
}

void *process_commands(char *wiredInstruction, int retFD) {
	vector<string> arguments;
	char *arg;
	unordered_map<string, int> commandToNumber;

	commandToNumber.insert(make_pair("CREATE", 1));
	commandToNumber.insert(make_pair("LIST", 2));
	commandToNumber.insert(make_pair("LEAVE", 3));
	commandToNumber.insert(make_pair("JOIN", 4));
	commandToNumber.insert(make_pair("ADD", 5));
	commandToNumber.insert(make_pair("NEWUSER", 6));
	commandToNumber.insert(make_pair("REPLY", 7));
	commandToNumber.insert(make_pair("TCP", 8));
	commandToNumber.insert(make_pair("UDP", 9));

	arg = strtok(wiredInstruction, "#@#");
	while (arg != NULL)
	{
		arguments.push_back(arg);
		arg = strtok(NULL, "#@#");
	}
	string command = arguments[0];
	string temp, ack;
	transform(command.begin(), command.end(), std::back_inserter(temp), ::toupper);
	command = temp;
	auto functionToBeCalled = commandToNumber.find(command);
	int sendFlag = 0;
	if (functionToBeCalled == commandToNumber.end())
		ack = "INVALID COMMAND!";
	else
	{
		string user = arguments[2];
		string ip = arguments[3];
		string port = arguments[4];
		
		auto tempItr = userToIPAndChatRoom.find(user);
		if (command.compare("NEWUSER") != 0 && tempItr == userToIPAndChatRoom.end())
			cout << "User not registered!";

		switch (functionToBeCalled->second)
		{
			case 1:
			{
				string chatRoomName = arguments[1];
				ack = createChatRoom(chatRoomName, user, ip, port);
				ack.push_back('\0');
				send(retFD, ack.c_str(), ack.size(), 0);
				break;
			}
			case 2:
			{
				string specification = arguments[1];
				ack = listFunctions(specification, user, ip, port);
				ack.push_back('\0');
				send(retFD, ack.c_str(), ack.size(), 0);
				break;
			}
			case 3:
				ack = leaveChatRoom(user, ip, port);
				ack.push_back('\0');
				send(retFD, ack.c_str(), ack.size(), 0);
				break;
			case 4:
			{
				string chatRoomName = arguments[1];
				ack = joinChatroom(chatRoomName, user, ip, port);
				ack.push_back('\0');
				send(retFD, ack.c_str(), ack.size(), 0);
				break;
			}
			case 5:
			{
				string userToBeAdded = arguments[1];
				auto it = userToIPAndChatRoom.find(user);
				string chatRoomName = it->second.second;
				ack = addUserToChatRoom(chatRoomName, userToBeAdded, ip, port);
				ack.push_back('\0');
				send(retFD, ack.c_str(), ack.size(), 0);
				break;
			}
			case 6:
			{
				string newUserName = arguments[2];
				// cout << "new user name " << newUserName << endl;
				ack = addNewUser(newUserName, user, ip, port);
				ack.push_back('\0');
				send(retFD, ack.c_str(), ack.size(), 0);
				break;
			}
			case 7:
			{
				string userName = arguments[2];
				vector<pair<string, string>> membersInSameRoom = replyMessage(userName, ip, port);

				if (membersInSameRoom.size() == 0){
					ack = "You do not belong to any chatroom!";
					ack.push_back('\0');
					send(retFD, ack.c_str(), ack.size(), 0);
				}
				
				else
				{
					sendFlag = 1;
					// cout << "chat before sending " << arguments[1] << endl;
					string flag = "chat";
					broadcast(membersInSameRoom, userName, arguments[1], flag);
				}
				
				break;
			}
			case 8:
			{
				string userName = arguments[2];
				vector<pair<string, string>> membersInSameRoom = replyMessage(userName, ip, port);
				// cout << "FILE WILL BE RECEIVED" << endl;
				ack = "SEND FILES\0";
				send(retFD, ack.c_str(), ack.size(), 0);
				receiveFile(retFD, ("./" + arguments[1]).c_str());
				string flag = "file";
				broadcast(membersInSameRoom, userName, "./" + arguments[1], flag);
				break;
			}
				
			case 9:
			{
				udpProtocol();
				break;
			}
			
		}
	
	}
	
	
	// if (sendFlag == 0)
	// 	send(retFD, ack.c_str(), ack.size(), 0);
}

void* user_to_server(void *sck) {
	int data_len;
	char data[MAX_DATA] = {'\0'};
	char * pch;

	string command, fstr, ack, client_alias;

	socketInfo si = *((socketInfo *)sck);
	//pthread_detach(pthread_self());
	cout << "New client from port " << ntohs(si.cli.sin_port) << " and IP " << inet_ntoa(si.cli.sin_addr) << '\n';
	
    vector<string> tempV;
    data_len = recv(si.nw, data, MAX_DATA, 0);
    
    cout << "MESSAGE FROM CLIENT: " << data << '\n';

    if(data_len)
		process_commands(data, si.nw);
    bzero(data, strlen(data));
	cout << "CLIENT CLOSED " << si.nw << '\n';
	close(si.nw);
	pthread_exit(0);
}

int main(int argc, char const *argv[]) {
	std::cout << "STARTING SERVER..." << '\n';
	fflush(stdin);
	fflush(stdout);
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t sockaddr_len = sizeof(struct sockaddr_in);
	socketInfo sck;

	pthread_t thread;

	server_ip = argv[1];
	server_port = argv[2];
	broadcast_port = argv[3];
	MAX_CLIENTS = atoi(argv[4]);

	signal(SIGINT, signalHandler_KillingThreads); //registers the signal handler
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
		perror("SERVER SOCKET: ");
		exit(-1);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(server_port));
	server.sin_addr.s_addr = inet_addr(server_ip);
	bzero(&server.sin_zero, 8);

	if ((bind(sock, (struct sockaddr*)&server, sockaddr_len)) == ERROR) {
		perror("BIND : ");
		exit(1);
	}

    // pthread_create(&thread, NULL, &heartBeat, NULL);
	// threadV.push_back(thread);
	
	cout << "SERVER STARTED..." << '\n';

	if ((listen(sock, MAX_CLIENTS)) == ERROR) {
		perror("LISTEN : ");
		exit(-1);
	}
    
	while(1) {
		if ((sck.nw = accept(sock, (struct sockaddr*)&client, &sockaddr_len)) == ERROR) {
			std::cout << "Can't connect with client " << inet_ntoa(client.sin_addr) << '\n';
		}
		else {
			sck.cli = client;
			pthread_create(&thread, NULL, &user_to_server, (void *) &sck);
			threadV.push_back(thread);
		}
	}
	close(sock);

    return 0;
}