/*
    Brian Nasralla

    The code below was adapted from the following source:
    https://github.com/jasonish/libevent-examples/blob/master/chat-server/chat-server.c
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* For inet_ntoa. */
#include <arpa/inet.h>

/* Required by event.h. */
#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

/* Libevent. */
#include <event.h>

#define MAX_SIZE 33 // The maximum length a fortune will be


/**
 * A struct for client specific data, also includes pointer to create
 * a list of clients.
 *
 * In event based programming it is usually necessary to keep some
 * sort of object per client for state information.
 */
struct client {
    char challenge[MAX_SIZE], missing_word[MAX_SIZE];
    char *guess;
    int wins, games_played;
    struct event_base *base;
    int client_fd;
	/* Events. We need 2 event structures, one for read event
	 * notification and the other for writing. */
	struct event game;
};

static void fortune(struct client *client)
{
    pid_t pid;
    int pipe_fort[2];
    char fortune[MAX_SIZE];
    char* buff = (char *) malloc(sizeof(char));
    // Initialise fortune buffer with EOF.
    for(int i = 0; i < MAX_SIZE; i++)
        fortune[i] = '\0';
    
    // Piping fortune
    if(pipe(pipe_fort) == -1)
    {
        perror("Unable to pipe");
        exit(1);
    }

    pid = fork();
    if(pid == -1)
    {
        perror("Could not fork");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0) //Child
    {
        int save_fd;
        save_fd = dup(STDOUT_FILENO);
        if(save_fd == -1)
        {
            perror("Error: dup call");
            exit(EXIT_FAILURE);
        }

        // Closing stdout fd
        close(STDOUT_FILENO);

        if(dup(pipe_fort[1]) == -1)
        {
            perror("Error: failed dup call");
            exit(EXIT_FAILURE);
        }

        char *args[] = { "fortune", "-n", "32", "-s" };
            // Execute fortune
        if(execvp(*args, args) == -1)
        {
            perror("Failed to execute fortune");
            exit(EXIT_FAILURE);
        }

        // Restoring stdout fd
        if(dup2(pipe_fort[1], save_fd))
        {
            perror("Error restoring stdout fd");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else //Parent
    {
        int wstatus;
        if(waitpid(0, &wstatus, WUNTRACED) == -1)
        {
            perror("Fortune: waitpid error");
            exit(EXIT_FAILURE);
        }
        read(pipe_fort[0], fortune, MAX_SIZE);
        buff = strcat(buff, fortune);
        
        strcpy(client->challenge, buff);
        free(buff);
    }
}

static void replace_word(struct client *client)
{
    int count, word_no;
    char *token, *position;
    char fortune[33];
    char delimit[] = " \t\r\n\v\f.,;!?~`_-"; // POSIX whitespace characters 
                                            // and other punctuation characters
    // count number of tokens
    position = client->challenge;
    while ((position = strpbrk(position, delimit)) != NULL) {
        position++; count++;    
    }

hide_word:
    // initialize randomizer and choose word
    srand(time(NULL));
    word_no = rand() % count;
    count = 0;

    // go through phrase word by word
    strcpy(fortune, client->challenge);
    token = strtok(fortune, delimit);
    while (token) {
        if (count == word_no) {
            // this word gets removed
            position = strstr(client->challenge, token);
            if (!position) {
                // something is wrong ... try again
                goto hide_word;
            }
            memset(position, '_', strlen(token));
            strcpy(client->missing_word, token);
            break;
        }
        count++;
        token = strtok(NULL, delimit);
    }
    if (token == NULL) {
        // this means that no word was chosen
        goto hide_word;
    }
}

/**
 * Set a socket to non-blocking mode.
 */
int
setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

    return EXIT_SUCCESS;
}

/**
 * This function will be called by libevent when the client socket is
 * ready for reading.
 */
void
on_read(int fd, short ev, void *arg)
{
	struct client *client = (struct client *)arg;
    char *buf, *word, message[100];
    char *delimit = " R:\t\r\n\v\f";
	int len;
	memset(message, 0, strlen(message));
	/* Because we are event based and need to be told when we can
	 * write, we have to malloc the read buffer and put it on the
	 * clients write queue. */
	buf = malloc(MAX_SIZE);
	if (buf == NULL)
		err(1, "malloc failed");

	len = read(fd, buf, MAX_SIZE);
	if (len == 0) {
		/* Client disconnected, remove the read event and the
		 * free the client structure. */
		printf("Client disconnected.\n");
                close(fd);
		event_del(&client->game);
		free(client);
		return;
	}
	else if (len < 0) {
		/* Some other error occurred, close the socket, remove
		 * the event and free the client structure. */
		printf("Socket failure, disconnecting client: %s",
		    strerror(errno));
		close(fd);
		event_del(&client->game);
		free(client);
		return;
	}

    if(strstr(buf, "Q:") == buf)
    {
        sprintf(message, "Q: You mastered %d/%d challenges. Good bye!\n", client->wins,client->games_played);
        if(write(fd, message, strlen(message)))
        {
            printf("Client disconnecting\n");
            close(client->client_fd);
            close(fd);
            event_del(&client->game);
            free(client);
            return;
        }
    }
    if(strstr(buf, "R:") == buf)
    {
        client->games_played++;
        memmove(buf, buf + 2, strlen(buf) - 2);
        word = strtok(buf, delimit);
        if(word == NULL || strcmp(word, client->missing_word) != 0)
        {
            sprintf(message, "F: Wrong guess - expected: %s\n", client->missing_word);
            if(write(fd, message, strlen(message)) == -1)
            {
                perror("Could not write");
                return;
            }
            client->guess = NULL;
        }
        else
        {
            client->wins++;
            sprintf(message, "O: Congradulations - challenge passed!\n");
            if(write(fd, message, strlen(message)) == -1)
            {
                perror("Could not write");
                return;
            }
        }
        fortune(client);
        replace_word(client);
        write(fd, "C: ", 4);
        if(write(fd, client->challenge, strlen(client->challenge)) == -1)
        {
            perror("Could not write");
            return;
        }
    }
    else
    {
        sprintf(message, "M: Unrecognised command. Please try again!\n");
        if(write(fd, message, strlen(message)) == -1)
        {
            perror("Could not write");
            return;
        }
        fortune(client);
        replace_word(client);
        write(fd, "C: ", 4);
        if(write(fd, client->challenge, strlen(client->challenge)) == -1)
        {
            perror("Could not write");
            return;
        }
    }
    
	/* Since we now have data that needs to be written back to the
	 * client, add a write event. */
	event_add(&client->game, NULL);
}

/**
 * This function will be called by libevent when the client socket is
 * ready for writing.
 */
void
on_write(int fd, short ev, void *arg)
{
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
void
on_accept(int fd, short ev, void *arg)
{
	int client_fd, ret;
    char message[100];
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	struct client *client;

	/* Accept the new connection. */
	client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd == -1) {
		warn("accept failed");
		return;
	}
	/* We've accepted a new client, allocate a client object to
	 * maintain the state of this client. */
	client = calloc(1, sizeof(*client));
	if (client == NULL)
    {
		perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    dup2(client_fd, client->client_fd);
	/* Set the client socket to non-blocking mode. */
	if (setnonblock(client_fd) < 0)
		warn("failed to set client socket non-blocking");

	/* Setup the read event, libevent will call on_read() whenever
	 * the clients socket becomes read ready.  We also make the
	 * read event persistent so we don't have to re-add after each
	 * read. */
	event_set(&client->game, client_fd, EV_READ|EV_PERSIST, on_read, client);

	/* Setting up the event does not activate, add the event so it
	 * becomes active. */
	event_add(&client->game, NULL);

	strcpy(message, "M: Guess the missing ____!\n");
    strcat(message, "M: Send your guess in the form ’R: word\\r\\n’.\n");

    ret = write(client_fd, message, strlen(message));
    if(ret == -1)
        perror("Could not write welcome message");

    fortune(client);
    replace_word(client);
    write(client_fd, "C: ", 4);
    ret = write(client_fd, client->challenge, strlen(client->challenge));
    if(ret == -1)
        perror("Could not write welcome message");
}

int
main(int argc, char **argv)
{
	int listen_fd;
	struct sockaddr_in listen_addr;
	int reuseaddr_on = 1;
	/* The socket accept event. */
	struct event ev_accept;

    if(argc != 2)
    {
        fprintf(stderr, "Usage error: Please supply a port number as an argument\n");
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        puts("Invalid port");
        return EXIT_FAILURE;
    }

	/* Initialize libevent. */
	event_init();

	/* Create our listening socket. This is largely boiler plate
	 * code that I'll abstract away in the future. */
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
		err(1, "listen failed");
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, 
		sizeof(reuseaddr_on)) == -1)
		err(1, "setsockopt failed");
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(port);
	if (bind(listen_fd, (struct sockaddr *)&listen_addr,
		sizeof(listen_addr)) < 0)
		err(1, "bind failed");
	if (listen(listen_fd, 5) < 0)
		err(1, "listen failed");

	/* Set the socket to non-blocking, this is essential in event
	 * based programming with libevent. */
	if (setnonblock(listen_fd) < 0)
		err(1, "failed to set server socket to non-blocking");

	/* We now have a listening socket, we create a read event to
	 * be notified when a client connects. */
	event_set(&ev_accept, listen_fd, EV_READ|EV_PERSIST, on_accept, NULL);
	event_add(&ev_accept, NULL);

	/* Start the libevent event loop. */
	event_dispatch();

	return EXIT_SUCCESS;
}