#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/signal.h>

pthread_mutex_t fileWriteLock;
pthread_mutex_t fileCreationLock;
FILE *logFile;
char outputPath[256], logFileName[] = "file_accepting_server.log";
size_t outputPathLen;
int serverSocket;

void interruptHandler(){
	close(serverSocket);
}

void writeToLog(int socket, const char *msg) {
	time_t now;
	time(&now);
	pthread_mutex_lock(&fileWriteLock);
	fprintf(logFile, "[%s] message from %i socket's handler: %s\n", ctime(&now), socket, msg);
	fflush(logFile);
	pthread_mutex_unlock(&fileWriteLock);
}

void *connection_handler(void *socket_desc) {
	int clientSocket = *(int *)socket_desc;
	int len;
	char *message, buffer[1024], pathBuffer[257];

	if ((len = recv(clientSocket, buffer, 1024, 0)) > 0) {
		if (outputPathLen + len > 256) {
			message = "Error, file name too long\n";
			send(clientSocket, message, strlen(message), 0);
			writeToLog(clientSocket, "too long filename received");
		} else {
			strcpy(pathBuffer, outputPath);
			strcpy(pathBuffer + outputPathLen, buffer);
			pathBuffer[outputPathLen + len - 1] = '\0';
			pthread_mutex_lock(&fileCreationLock);
			if (access(pathBuffer, F_OK) == 0) {
				pthread_mutex_unlock(&fileCreationLock);
				message = "Error, file exists\n";
				send(clientSocket, message, strlen(message), 0);
				writeToLog(clientSocket, "error creating file, file exists");
			} else {
				FILE *outputFile = fopen(pathBuffer, "w");
				pthread_mutex_unlock(&fileCreationLock);
				if (outputFile != NULL) {
					message = "Accepted\n";
					if (send(clientSocket, message, strlen(message), 0) > 0) {
						while ((len = recv(clientSocket, buffer, 1024, 0)) > 0) {
							if (fwrite(buffer, len, 1, outputFile) != 1) {
								message = "Error occurred while writing to file\n";
								send(clientSocket, message, strlen(message), 0);
								writeToLog(clientSocket, "Error occurred while writing to file");
								break;
							}
						}
					} else
						len = -3;
					fclose(outputFile);
				} else
					len = -2;
			}
		}
		if (len == 0) {
			message = "File received successfully\n";
			send(clientSocket, message, strlen(message), 0);
			writeToLog(clientSocket, "file received successfully");
		} else if (len == -1) {
			writeToLog(clientSocket, strerror(errno));
		} else if (len == -2) {
			writeToLog(clientSocket, "error occurred while creating file");
		} else if (len == -3) {
			writeToLog(clientSocket, "error sending accept message to clientSrc");
		}
	}
	free(socket_desc);
	return 0;
}

int main(int argc, char const *argv[]) {
	int port = 31337;
	const char *portArg = NULL, *pathArg = NULL;
	if (argc != 1 && argc != 3 && argc != 5) {
	invalidArgs:
		fprintf(stderr, "Incorrect input arguments\n"
						"This program expect launching next way:\n"
						"	fileAcceptingServer [-port <port>] [-path <path>]\n"
						"Where:"
						"	<port> should be valid port number\n"
						"	<path> should be valid path\n");
		return 1;
	}
	for (int i = 1; i < argc; i += 2) {
		if (strlen(argv[i]) != 5)
			goto invalidArgs;
		switch (argv[i][2]) {
			case 'o':
				if (portArg)
					goto invalidArgs;
				portArg = argv[i + 1];
				break;
			case 'a':
				if (pathArg)
					goto invalidArgs;
				pathArg = argv[i + 1];
				break;
			default:
				goto invalidArgs;
		}
	}
	if (portArg != NULL && (sscanf(portArg, "%i", &port) != 1 || port < 1 || port > 65535))
		goto invalidArgs;
	if (pathArg) {
		if ((outputPathLen = strlen(pathArg)) > 255)
			goto invalidArgs;
		strcpy(outputPath, pathArg);
		if (outputPath[outputPathLen - 1] != '/') {
			outputPath[outputPathLen++] = '/';
			outputPath[outputPathLen] = '\0';
		}
		mkdir(outputPath, S_IRWXU);
	}

	// initializing socket
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1
			|| bind(serverSocket, (struct sockaddr *)&server, sizeof(server)) == -1) {
		perror("Server socket init failed");
		return 1;
	}
	// creating log file
	size_t logFileLen = strlen(logFileName);
	char buffer[257];
	strcpy(buffer, outputPath);
	if (logFileLen + outputPathLen > 256) {
		fprintf(stderr, "Log file path is too long");
		close(serverSocket);
		return 1;
	}
	strcpy(buffer + outputPathLen, logFileName);
	// creating log file
	if ((logFile = fopen(buffer, "w")) == NULL) {
		perror("Error creating log file");
		close(serverSocket);
		return 1;
	}
	// initializing mutexes
	if (pthread_mutex_init(&fileCreationLock, NULL) || pthread_mutex_init(&fileWriteLock, NULL)) {
		fprintf(stderr, "Error creating mutexes\n");
		fclose(logFile);
		close(serverSocket);
		return 1;
	}
	// starting to listen socket
	if (listen(serverSocket, 64)) {
		perror("Server socket listening error");
		pthread_mutex_destroy(&fileCreationLock);
		pthread_mutex_destroy(&fileWriteLock);
		fclose(logFile);
		close(serverSocket);
		return 1;
	}
	printf("Server started successfully\n");
	signal(SIGINT, interruptHandler);
	signal(SIGTERM, interruptHandler);
	// starting accepting new connections
	int newSocket, *clientSocket;
	while ((newSocket = accept(serverSocket, NULL, NULL)) != -1) {
		clientSocket = malloc(1);
		*clientSocket = newSocket;
		pthread_t thread;
		if (pthread_create(&thread, NULL, connection_handler, (void *)clientSocket)) {
			free(clientSocket);
			fprintf(stderr, "Error creating thread for new socket\n");
			continue;
		}
		if (pthread_detach(thread))
			pthread_join(thread, NULL);
	}

	pthread_mutex_destroy(&fileCreationLock);
	pthread_mutex_destroy(&fileWriteLock);
	fclose(logFile);
	printf("Server stopped\n");
	return 0;
}
