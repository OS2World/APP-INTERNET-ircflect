#ifndef BSD_SELECT
#define BSD_SELECT
#endif


#include <types.h>
#include <netinet\in.h>
#include <sys\socket.h>
#include <sys\select.h>
#include <netdb.h>
#include <stdio.h>
#include <process.h>

#define INCL_DOSPROCESS
#include <os2.h>

#include <mt.h>

#include <nerrno.h>
#include <ctype.h>
#include <os2def.h>


#ifndef errno
extern int errno;
#endif


void thread2(void *);

typedef struct {
  int sock;
  int oldsock;
  char host_name[255];
  int host_port;

} NEW_CLIENT;



main(argc, argv)
int argc; char *argv[];
{
	TID 				thread2Tid;

	NEW_CLIENT *new;

	struct in_addr tmpaddr;
	struct sockaddr_in server;
	struct hostent *hp, *gethostbyname();

	int length = sizeof(struct sockaddr_in);
	struct sockaddr_in saddr;

	unsigned char buf[1024];
	int insock,  tmpsock;
	int insocknum, outsocknum;
	int one=1;
	fd_set needread;		/* for seeing which fds we need to read from */



	/* Check Arguments */

	if (argc < 4) {
	printf("usage: %s <in port> <out port> <machine> <client file> <server file>\n",argv[0]);
	exit(1);
	}

	insocknum = atoi(argv[1]);
	outsocknum = atoi(argv[2]);


	/* Create listening post on pseudo-port */

	insock = socket(AF_INET, SOCK_STREAM, 0);



	if (insock < 0) {
	printf("opening pseudo-port stream socket");
	exit(1);
	}

	memmove((char *) &tmpaddr, (char *) &(server.sin_addr),
	  sizeof(struct in_addr));

	/* Name socket using user supplied port number */

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = (unsigned short) htons((unsigned short) insocknum);
	setsockopt(insock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));

	if (bind(insock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)))
		{
		printf("binding pseudo-port stream socket");
		exit(1);
		}


	/* Start accepting connections */
	if (listen(insock, 5) == -1)
		{
		printf("Os2Chat: Error listening");
		exit(1);
		}



	/* Every connection spawns a child to handle the communication */


   while(1)
   {
		 DosSleep(50);
		 FD_ZERO(&needread);
		 FD_SET(insock, &needread);
//		   setfds(&needread);

		 if (select(2048, &needread, (fd_set *)0,
			  (fd_set *)0, 0) == -1)
		   {
			if (sock_errno() != EINTR)
				{
				 continue;
				}
			else
			 continue; /* do it again and get it right */
		   }

		if (FD_ISSET(insock, &needread))
		{
		tmpsock = accept(insock, (struct sockaddr *) &saddr, &length);
		if (tmpsock == -1) { printf("accept %d",sock_errno()); exit(1); }

		new = malloc(sizeof(NEW_CLIENT));
		new->sock=tmpsock;
		new->oldsock=insock;
		sprintf(new->host_name,argv[3]);
		new->host_port=outsocknum;


		thread2Tid= _beginthread(thread2, 0, STACK_SIZE,  new);
//		  soclose(insock);
		}
	/* Close listening post - it's now called "msgsock" */

	}
}


void thread2(void *args) { NEW_CLIENT *player = (NEW_CLIENT *) args; int
rval;

	int msgsock,outsock,insock,outsocknum;
	struct hostent *hp, *gethostbyname();
	struct sockaddr_in server;
    unsigned char buf[1024],buf2[1024];

	int one=1;
	fd_set fdset;

	msgsock=player->sock;
	insock=player->oldsock;
	outsocknum=player->host_port;

    sprintf(buf2,"DCC");

//	  soclose(insock);

	/* Create output socket to server */
	printf("opening connect to site\r\n");

	outsock = socket(AF_INET, SOCK_STREAM, 0);
	if (outsock < 0)
		{
		 printf("opening server stream socket");
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);

		 _endthread();
		}


	/* Connect socket using machine & port specified on command line. */

	server.sin_family = AF_INET;
	hp = gethostbyname(player->host_name);
	if (hp == 0)
		{
		fprintf(stderr, "%s: unknown machine\n", player->host_name);
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);

		_endthread();
		}

	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port = (unsigned short) htons((unsigned short)outsocknum);

	if (connect(outsock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0)
		{
		printf("connecting server stream socket");
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);

		_endthread();
		}
	printf("Connected..!!\r\n");

	/* Open the output files */


	do {

	/* find out who's talking */

		FD_ZERO(&fdset);
		FD_SET(msgsock, &fdset);
		FD_SET(outsock, &fdset);
		 if (select(2048, &fdset, 0,0, 0) == -1)
			{
			printf("select");
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);

			_endthread();
			}

	memset(buf,0,1024);

	if (FD_ISSET(msgsock, &fdset) && FD_ISSET(outsock, &fdset))
		fprintf(stderr,"Two talkers - no listeners\n");


	/* Client is talking to server */

	if (FD_ISSET(msgsock, &fdset)) {
		if ((rval = recv(msgsock, buf, 1024,0x0)) < 0)
		{
			printf("error %d \r\n",sock_errno());
			printf("reading stream message");
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);
			_endthread();

			break;
		}
		if (rval == 0)
		fprintf(stderr,"Ending client connection\n");
		else {

        if ( strstr(buf,buf2) )  printf("C:%s\r\n",buf);
		if (send(outsock, buf, rval,0x0) < 0)
			printf("writing on output stream socket");

		}
		}

	/* Server is talking to client */

	else {
		if (! FD_ISSET(outsock, &fdset)) {
		printf("weird behavior");
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);

		_endthread();

		}
		if ((rval = recv(outsock, buf, 1024,0x0)) < 0)
		{
			printf("error %d \r\n",sock_errno());
			printf("reading stream message");
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);
			_endthread();

			break;
		}
		if (rval == 0)
		fprintf(stderr,"Ending server connection\n");
		else {
        if ( strstr(buf,buf2) )  printf("S:%s\r\n",buf);

		if (send(msgsock, buf, rval,0x0) < 0)
			printf("writing on output stream socket");
		}
		}
	} while (rval != 0);


	/* Close up shop */

	fprintf(stderr, "Closing Connections\n");
			shutdown(msgsock,2);
			shutdown(outsock,2);
			soclose(msgsock);
			soclose(outsock);

			free(player);
			_endthread();

}

