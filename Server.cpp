#include <WinSock2.h>
#include <stdio.h>
#include <iostream>
#include <WS2tcpip.h>
#include <process.h>
#include <ctime>
#include "fs.h"

#pragma comment(lib, "Ws2_32.lib")

#define PORT "8080"
#define C_BUFFER_SIZE 40
#define MAX_BUFFER_SIZE 1024

struct tcp_header {
	unsigned int max_bufsize;
	unsigned int tcph_seqnum;
	unsigned int tcph_acknum;
	unsigned int
		tcph_fin : 1,
		tcph_syn : 1,
		tcph_rst : 1,
		tcph_psh : 1,
		tcph_ack : 1,
		tcph_urg : 1;
	unsigned short int tcph_checksum;
	char msg[];
};

void clearFlags(struct tcp_header* header) {
	header->tcph_fin = 0, header->tcph_syn = 0, header->tcph_rst = 0, header->tcph_psh = 0, header->tcph_ack = 0, header->tcph_urg = 0;
}

unsigned int payloadSize(unsigned int bufferSize) {
	return (bufferSize - sizeof(tcp_header));
}

unsigned short int chksum(struct tcp_header* header) {
	short int sum = 0;
	sum += header->tcph_seqnum;
	sum += header->tcph_acknum;
	sum += atoi(header->msg);
	sum += header->tcph_fin;
	sum += header->tcph_syn;
	sum += header->tcph_rst;
	sum += header->tcph_psh;
	sum += header->tcph_ack;
	sum += header->tcph_urg;
	return sum;
}

void freeBuffers(struct tcp_header* send, struct tcp_header* recv) {
	delete (send);
	delete (recv);
}


unsigned __stdcall ClientSession(void* data) {
	SOCKET ClientSocket = (SOCKET)data;
	int iResult;
	// Process the client
	// Perform 3 way handshake
	struct tcp_header* recvbuf = new tcp_header;
	struct tcp_header* sendbuf = new tcp_header;
	int iSendResult;
	int nextSeq;
	unsigned int clientBufSize;
	unsigned int bufferSize = MAX_BUFFER_SIZE;
	time_t start;
	// TCP Handshake Process:
	// Receive SYN from Client:
	printf("Waiting for SYN...\n");
	iResult = recv(ClientSocket, (char*)recvbuf, C_BUFFER_SIZE, 0);
	if (iResult == SOCKET_ERROR) {
		printf("recv() failed with error code: %d\n", WSAGetLastError());
		WSACleanup();
		freeBuffers(sendbuf, recvbuf);
		return 1;
	}
	else if (recvbuf->tcph_syn == 1) {
		clientBufSize = recvbuf->max_bufsize;
		printf("Received SYN from Client.\nClient SN: %d\n", recvbuf->tcph_seqnum);
	}
	else {
		printf("SYN not received. Reject.\n");
		freeBuffers(sendbuf, recvbuf);
		WSACleanup();
		return 1;
	}
	sendbuf->tcph_syn = 1;
	sendbuf->tcph_ack = 1;
	sendbuf->tcph_seqnum = 1;
	sendbuf->tcph_acknum = recvbuf->tcph_seqnum + 1, nextSeq = 2;
	printf("Sending: syn flag: %d, ack flag: %d, seqnum: %d, acknum: %d\n", sendbuf->tcph_syn, sendbuf->tcph_ack,
		sendbuf->tcph_seqnum, sendbuf->tcph_acknum);
	iSendResult = send(ClientSocket, (char*)sendbuf, C_BUFFER_SIZE, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send() failed with error code: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		freeBuffers(sendbuf, recvbuf);
		WSACleanup();
		return 1;
	}
	else
		printf("Sent SYN-ACK to Client.\n");
	// Receive ACK from Server and authenticate it:
	printf("Waiting for ACK from Client...\n");
	time(&start);
	for (;;) {
		iResult = recv(ClientSocket, (char*)recvbuf, C_BUFFER_SIZE, 0);
		if (time(NULL) > start + 10) {
			printf("10s receive timeout waiting for ACK (3way handshake)\n");
			closesocket(ClientSocket);
			freeBuffers(recvbuf, sendbuf);
			WSACleanup();
			return 1;
		}
		if (iResult == SOCKET_ERROR) {
			printf("recv() failed with error code: %d\n", WSAGetLastError());
			WSACleanup();
			freeBuffers(sendbuf, recvbuf);
			return 1;
		}
		else if (recvbuf->tcph_ack == 1 && recvbuf->tcph_syn == 0 && recvbuf->tcph_acknum == nextSeq) {
			printf("Received ACK from Client.\nConnection Established.\n");
			break;
		}
		else {
			printf("Incorrect ACK received. Rejecting connection...");
			closesocket(ClientSocket);
			WSACleanup();
			freeBuffers(sendbuf, recvbuf);
			return 1;
		}
	}
	// Allocate payload and buffersize
	if (clientBufSize < MAX_BUFFER_SIZE)
		bufferSize = clientBufSize;
	else
		bufferSize = MAX_BUFFER_SIZE;
	// Max payload size: (Size of my character array)
	int payload = payloadSize(bufferSize);
	printf("My payload size is: %ld\n", payload);
	// Connection is established: Can now accept commands for fileserver.
    string command;
	char help[900] = "Please do not use the following characters: ^ & | * @\nAlso please do not use whitespace or numbers in file or directory names.\nUsage:\ntouch filename numblocks\n mkdir filename\ncd directoryname\n vi filename newFileText\nappend filename textToAppend\nprint: this will print the file contents to the screen\nType help to show this message again\n";
	memcpy(sendbuf->msg, help, payload);
	iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
	if (iResult == SOCKET_ERROR) {
		printf("send error at code: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		freeBuffers(sendbuf, recvbuf);
		WSACleanup();
		return 1;
	}
	printf("Sent help message to client.\n");

    //cout << endl;
    disc_em disc = disc_em("disc.txt");
    bool operating = true;
    while (operating) {
        //getline(cin, command);
		recv(ClientSocket, (char*)recvbuf, bufferSize, 0);
		command = recvbuf->msg;
        if (command.substr(0, 5) == "touch") {
            string fname = "", text = "";
            int i = 6;
            while (command[i] != ' ') {
                fname += command[i];
                i++;
            }
            i++;
            while (i < command.length()) {
                text += command[i];
                i++;
            }
            //cout << disc.touch(fname, stoi(text));
            //disc.print();
			string temp = disc.touch(fname, stoi(text));
			memcpy(sendbuf->msg, temp.c_str(), payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
			printf("Sent response for touch.\n");
        }
        if (command.substr(0, 6) == "append") {
            string fname = "", text = "";
            int i = 7;
            while (command[i] != ' ') {
                fname += command[i];
                i++;
            }
            i++;
            while (i < command.length()) {
                text += command[i];
                i++;
            }
            //cout << disc.append(fname, text);
            //disc.print();
			string temp = disc.append(fname, text);
			memcpy(sendbuf->msg, temp.c_str(), payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
			printf("Sent response for append.\n");
        }
        else if (command.substr(0, 5) == "mkdir") {
            string dname = "";
            int i = 6;
            while (i < command.length()) {
                dname += command[i];
                i++;
            }
            //cout << disc.mkdir(dname);
            //disc.print();
			string temp = disc.mkdir(dname);
			memcpy(sendbuf->msg, temp.c_str(), payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
        }
        else if (command.substr(0, 2) == "ls") {
            //cout << disc.ls();
			string temp = disc.ls();
			memcpy(sendbuf->msg, temp.c_str(), payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
			printf("Sent response for mkdir.\n");
        }
        else if (command.substr(0, 2) == "cd") {
            string dName = "";
            for (int i = 3; i < command.length(); i++) {
                dName += command[i];
            }
            //cout << disc.cd(dName);
			string temp = disc.cd(dName);
			memcpy(sendbuf->msg, temp.c_str(), payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
			printf("Sent response for cd.\n");
        }
        else if (command.substr(0, 2) == "vi") {
            string fname = "", text = "";
            int i = 3;
            while (command[i] != ' ') {
                fname += command[i];
                i++;
            }
            i++;
            while (i < command.length()) {
                text += command[i];
                i++;
            }
            //cout << disc.vi(fname, text);
            //disc.print();
			string temp = disc.vi(fname, text);
			memcpy(sendbuf->msg, temp.c_str(), payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
			printf("Sent response for vi.\n");
        }
        else if (command == "exit") {
            operating = false;
            cout << "Disc turning off\n";
        }
        else if (command == "help") {
			memcpy(sendbuf->msg, help, payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
			if (iResult == SOCKET_ERROR) {
				printf("send error at code: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				freeBuffers(sendbuf, recvbuf);
				WSACleanup();
				return 1;
			}
        }
        else if (command == "print") {
            //disc.print();
			string temp = disc.print();
			memcpy(sendbuf->msg, temp.c_str(), payload);
			iResult = send(ClientSocket, (char*)sendbuf, bufferSize, 0);
			printf("Sent response for print.\n");
        }
        cout << endl;
    }
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("Shutdown of send failed: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	closesocket(ClientSocket);
	WSACleanup();
}

int main() {
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup Failed: %d\n", iResult);
	}

	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo("127.0.0.1", PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("Bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result); // No longer needed

	if (listen(ListenSocket, 4) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Here is where the connection is accepted. For multiple clients, threads will be created to accept clients.
	SOCKET ClientSocket;
	while ((ClientSocket = accept(ListenSocket, NULL, NULL))) {
		// Create a new thread for the accept client.
		unsigned threadID;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)ClientSocket, 0, &threadID);
	}


	// Disconnect the Server:


	return 0;

}
