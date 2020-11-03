#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")
#define PORT "8080"
#define SERVER_ADDRESS "127.0.0.1"
#define CONTROL_BUFFER_SIZE 40
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

int payloadSize(unsigned int bufferSize) {
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
	delete send;
	delete recv;
}

int main() {
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup Failed: %d\n", iResult);
	}


	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(SERVER_ADDRESS, PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;
	ptr = result;
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Error at socket: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}
	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server.\n");
		WSACleanup();
		return 1;
	}

	int timeout = 15000; // 10 seconds
	iResult = setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	if (iResult == SOCKET_ERROR) {
		printf("setsockopt failed with error code: %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("sockopt timeout is set for: %d ms\n", timeout);

	// Set socket to non-blocking: Then I can use my own timeouts for receiving...
	u_long mode = 1;
	//ioctlsocket(ConnectSocket, FIONBIO, &mode);
	// Create time_t object used for send/recv loop timeouts
	time_t start;
	//std::time(&timer); Stores current time in timer.

	struct tcp_header* recvbuf = new tcp_header;
	struct tcp_header* sendbuf = new tcp_header;
	// 3-Way Handshake
	// Set sendbuf for SYN to server:
	sendbuf->tcph_syn = 1;
	sendbuf->tcph_seqnum = 1;
	sendbuf->max_bufsize = MAX_BUFFER_SIZE;
	int nextSeqnum = 1;
	unsigned int serverBufSize;
	unsigned int bufferSize = MAX_BUFFER_SIZE;
	//Sleep(500);
	iResult = send(ConnectSocket, (char*)sendbuf, CONTROL_BUFFER_SIZE, 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error code: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		freeBuffers(sendbuf, recvbuf);
		WSACleanup();
		return 1;
	}
	else
		printf("Syn sent to server: %d\n", sendbuf->tcph_seqnum);

	// Receive SYN-ACK : Need to add a timeout if the SYN-ACK is not received...
	printf("Waiting for SYN-ACK...\n");
	time(&start);
	for (;;) {
		iResult = recv(ConnectSocket, (char*)recvbuf, CONTROL_BUFFER_SIZE, 0);
		if (time(NULL) > start + 10) {
			printf("10s Received timeout waiting for SYN-ACK!\n");
			closesocket(ConnectSocket);
			freeBuffers(sendbuf, recvbuf);
			WSACleanup();
			return 1;
		}
		break;
	}
	printf("Received: syn flag: %d, ack flag: %d, seqnum: %d, acknum: %d\n", recvbuf->tcph_syn, recvbuf->tcph_ack,
		recvbuf->tcph_seqnum, recvbuf->tcph_acknum);
	printf("Current sendbuf seq num: %d\n", sendbuf->tcph_seqnum);
	if (iResult == SOCKET_ERROR) {
		printf("recv failed with error code: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		freeBuffers(sendbuf, recvbuf);
		WSACleanup();
		return 1;
	}
	// Confirm syn-ack flags and ack number
	else if (recvbuf->tcph_syn == 1 and recvbuf->tcph_ack == 1 and recvbuf->tcph_acknum == nextSeqnum + 1) {
		printf("Received SYN-ACK from Server.\n");
		serverBufSize = recvbuf->max_bufsize;
	}
	else {
		printf("SYN-ACK not received. Connection rejected.");
		closesocket(ConnectSocket);
		freeBuffers(sendbuf, recvbuf);
		WSACleanup();
		return 1;
	}

	// Send ACK to server: ack flag and ack number.
	clearFlags(sendbuf);
	sendbuf->tcph_ack = 1;
	sendbuf->tcph_acknum = recvbuf->tcph_seqnum + 1;
	iResult = send(ConnectSocket, (char*)sendbuf, CONTROL_BUFFER_SIZE, 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error code: %d\n", WSAGetLastError());
		freeBuffers(sendbuf, recvbuf);
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	else
		printf("Sent ACK to Server: %d\n", sendbuf->tcph_acknum);
	printf("Connection established.\n");
	// Evaluate buffer sizes and malloc buffers:
	if (serverBufSize < MAX_BUFFER_SIZE)
		bufferSize = serverBufSize;
	else
		bufferSize = MAX_BUFFER_SIZE;
	// Max payload size: (Size of my character array)
	//freeBuffers(sendbuf, recvbuf);
	unsigned int payload = payloadSize(bufferSize);
	printf("Max payload size is: %ld\n", payload);
	char cmdBuffer[100] = "";
	// Send a message to the Server:
	// Receive help message from server:
	iResult = recv(ConnectSocket, (char*)recvbuf, bufferSize, 0);
	if (iResult == SOCKET_ERROR) {
		printf("recv failed with error code: %d\n", WSAGetLastError());
		freeBuffers(sendbuf, recvbuf);
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	printf("%s", recvbuf->msg);
	// Send command to server:
	bool operating = "true";
	while (operating) {
		printf("Please enter a command:\n");
		std::cin.sync();
		std::cin.getline(cmdBuffer, 100);
		memcpy(sendbuf->msg, cmdBuffer, payload);
		iResult = send(ConnectSocket, (char*)sendbuf, bufferSize, 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error code: %d\n", WSAGetLastError());
			freeBuffers(sendbuf, recvbuf);
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
		iResult = recv(ConnectSocket, (char*)recvbuf, bufferSize, 0);
		printf("%s\n", recvbuf->msg);
	}
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("Shutdown failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		freeBuffers(sendbuf, recvbuf);
		WSACleanup();
		return 1;
	}
	closesocket(ConnectSocket);
	freeBuffers(sendbuf, recvbuf);
	WSACleanup();

	return 0;
}
