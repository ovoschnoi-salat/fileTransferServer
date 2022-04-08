#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main() {
	int sock, port;
	struct sockaddr_in server;
	char buffer[BUFFER_SIZE], *msg;

	printf("Host: ");
	if (scanf("%s", buffer) != 1) {
		msg = "Error reading host\n";
		goto error;
	}
	printf("Port: ");
	if (scanf("%i", &port) != 1) {
		msg = "Error reading port\n";
		goto error;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Could not create socket: ");
		msg = strerror(errno);
		goto error;
	}
	server.sin_addr.s_addr = inet_addr(buffer);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("Connection failed: ");
		msg = strerror(errno);
		goto error1;
	}
	printf("filename: ");
	if (scanf("%255s", buffer) != 1) {
		msg = "Error reading filename\n";
		goto error1;
	}
	FILE *inputFile = fopen(buffer, "r");
	if (inputFile == NULL) {
		msg = "Error opening file\n";
		goto error1;
	}
	printf("filename on serverSrc: ");
	if (scanf("%255s", buffer) != 1) {
		msg = "Error reading filename\n";
		goto error2;
	}
	int len = strlen(buffer);
	buffer[len] = '\n';
	if (send(sock, buffer, len + 1, 0) <= 0) {
		msg = "Error sending file name\n";
		goto error2;
	}
	if ((len = recv(sock, buffer, 100, 0)) <= 0) {
		msg = "Error receiving answer from serverSrc\n";
		goto error2;
	}
	if (buffer[0] == 'A') {
		int sent;
		while ((len = fread(buffer, 1, BUFFER_SIZE, inputFile)) > 0) {
			if ((sent = send(sock, buffer, len, 0)) != len) {
				printf("Error sending file to serverSrc: ");
				if (sent == -1)
					msg = strerror(errno);
				else
					msg = "buffer sent partly\n";
				goto error2;
			}
		}
		if (len == -1) {
			msg = strerror(errno);
			goto error2;
		}
		shutdown(sock, SHUT_WR);

		if ((len = recv(sock, buffer, 100, 0)) <= 0) {
			msg = "Error receiving answer from serverSrc\n";
			goto error2;
		}
		buffer[len] = '\0';
		printf("Server's reply: %s", buffer);
	} else {
		printf("Server's reply: ");
		msg = buffer;
		buffer[len] = '\0';
		goto error2;
	}
	fclose(inputFile);
	close(sock);
	return 0;

error2:
	fclose(inputFile);
error1:
	close(sock);
error:
	printf("%s", msg);
	return 1;
}
