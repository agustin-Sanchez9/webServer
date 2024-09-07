/*
 * clifcgi1.c
 * 
 * Copyright 2024 osboxes <osboxes@osboxes>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * para compilar:
 * $ gcc -Wall -o clifcgi2 clifcgi2.c -lfcgi
 */
#define _GNU_SOURCE  //declara environ  en unistd.h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>

#include <fcgimisc.h>
#include <fcgiapp.h>
#include <fastcgi.h>
#include <fcgios.h>


#define MAX 1024
#define PORT 8282
#define SA struct sockaddr

/*
 * Simple buffer (not ring buffer) type, used by all event handlers.
 */
#define BUFFLEN 8192
typedef struct {
    char *next;
    char *stop;
    char buff[BUFFLEN];
} Buffer;


void func(int sockfd);
FCGI_Header MakeHeader(
        int type,
        int requestId,
        int contentLength,
        int paddingLength);
FCGI_BeginRequestBody MakeBeginRequestBody(
        int role,
        int keepConnection);
void FCGIUtil_BuildNameValueHeader(
        int nameLen,
        int valueLen,
        unsigned char *headerBuffPtr,
        int *headerLenPtr);
void ScheduleIo(int sock);        
static void WriteStdinEof(void);
static void FCGIexit(int sockfd,int exitCode);
static void AppServerWriteHandler(ClientData dc, int bytesWritten);
static void AppServerReadHandler(ClientData dc, int bytesRead);
static int GetPtr(char **ptr, int n, Buffer *pBuf);
static void WebServerReadHandler(ClientData dc, int bytesRead);

static int wsReadPending = 0;
static int fcgiReadPending = 0;
static int fcgiWritePending = 0;
static int bytesToRead;    /* number of bytes to read from Web Server */
static int exitStatus = 0;
static int exitStatusSet = FALSE;

static int stdinFds[3];
static Buffer fromAS;      /* Bytes read from the FCGI application server. */
static Buffer fromWS;   /* Buffer for data read from Web server
                         * and written to FastCGI application. Used
                         * by WebServerReadHandler and
                         * AppServerWriteHandler. */
static int webServerReadHandlerEOF;
                        /* TRUE iff WebServerReadHandler has read EOF from
                         * the Web server. Used in main to prevent
                         * rescheduling WebServerReadHandler. */
                         
 
static FCGI_Header header; /* Header of the current record.  Is global
                            * since read may return a partial header. */
static int headerLen = 0;  /* Number of valid bytes contained in header.
                            * If headerLen < sizeof(header),
                            * AppServerReadHandler is reading a record header;
                            * otherwise it is reading bytes of record content
                            * or padding. */
static int contentLen;     /* If headerLen == sizeof(header), contentLen
                            * is the number of content bytes still to be
                            * read. */
static int paddingLen;     /* If headerLen == sizeof(header), paddingLen
                            * is the number of padding bytes still
                            * to be read. */
static int requestId;      /* RequestId of the current request.
                            * Set by main. */
static FCGI_EndRequestBody erBody;
static int readingEndRequestBody = FALSE;
                           /* If readingEndRequestBody, erBody contains
                            * partial content: contentLen more bytes need
                            * to be read. */
int fdServerSoc=0; // socket global
 
int main(int argc,char *argv[]) {
    int sockfd;
    struct sockaddr_in servaddr;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
 
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
 
    // function for chat
    func(sockfd);
 
    // close the socket
    close(sockfd);
}

void func(int sockfd)
{
    char **envp = environ;
    int count;
    FCGX_Stream *paramsStream;
    unsigned char headerBuff[8];
    int headerLen, valueLen;
    char *equalPtr;
    FCGI_BeginRequestRecord beginRecord;	
	
	
	requestId = 1;
	fdServerSoc=sockfd;
	
	
    /*
     * Send FCGI_BEGIN_REQUEST (XXX: hack, separate write)
     */
    beginRecord.header = MakeHeader(FCGI_BEGIN_REQUEST, requestId,
            sizeof(beginRecord.body), 0);
    beginRecord.body = MakeBeginRequestBody(FCGI_RESPONDER, FALSE);
    count = OS_Write(sockfd, (char *)&beginRecord, sizeof(beginRecord));
    if(count != sizeof(beginRecord)) {
		printf("func(): error count=%d sizeof(beginRecord)=%ld\n",count,sizeof(beginRecord));
        exit(OS_Errno);
    }	
	
    /*
     * Send environment to the FCGI application server
     */
    char *pvar[4] = { "CONTENT_LENGTH=19", "CONTENT_TYPE=text/xml;charset=utf-8", "QUERY_STRING=first=Zara&last=Ali", NULL };
    envp = pvar;
    paramsStream = FCGX_CreateWriter(sockfd, requestId, 8192, FCGI_PARAMS);
    for( ; *envp != NULL; envp++) {
        equalPtr = strchr(*envp, '=');
        if(equalPtr  == NULL) {
            exit(1000);
        }
        valueLen = strlen(equalPtr + 1);
		printf("func() envp=[%s] valueLen=%d\n",*envp,valueLen);
        FCGIUtil_BuildNameValueHeader(
                equalPtr - *envp,
                valueLen,
                &headerBuff[0],
                &headerLen);
        if(FCGX_PutStr((char *) &headerBuff[0], headerLen, paramsStream) < 0
                || FCGX_PutStr(*envp, equalPtr - *envp, paramsStream) < 0
                || FCGX_PutStr(equalPtr + 1, valueLen, paramsStream) < 0) {
            exit(FCGX_GetError(paramsStream));
        }
    }
    FCGX_FClose(paramsStream);
    FCGX_FreeStream(&paramsStream);	
	
	/*
     * Perform the event loop until AppServerReadHander sees FCGI_END_REQUEST
     */
    fromWS.stop = fromWS.next = &fromWS.buff[0];
    webServerReadHandlerEOF = FALSE;
    OS_SetFlags(sockfd, O_NONBLOCK);

    if (bytesToRead <= 0)
		WriteStdinEof();
printf("1\n");
    ScheduleIo(sockfd);
printf("2\n");

    while(!exitStatusSet) {
        /*
	 * NULL = wait forever (or at least until there's something
	 *        to do.
	 */
printf("3\n");
        OS_DoIo(NULL);
    }
printf("4\n");
    if(exitStatusSet) {
        FCGIexit(sockfd,exitStatus);
    } else {
        FCGIexit(sockfd,999);
    }	
	
	/*
    char buff[MAX];
    int n;
	bzero(buff, sizeof(buff));
	n = read(sockfd, buff, sizeof(buff));
	printf("Lei(1) %d bytes From Server : [%s]",n,buff);
	sleep(2);
	bzero(buff, sizeof(buff));
	n = read(sockfd, buff, sizeof(buff));
	printf("Lei(2) %d bytes From Server : [%s]",n,buff);
	*/
	/*
    for (;;) {
        bzero(buff, sizeof(buff));
        n = 0;
        strcpy(buff,"POST /fcgi1 HTTP/1.1\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: 19\r\nConnection: Keep-Alive\r\n\r\nfirst=Zara&last=Ali");
        
        printf("strlen(buff)=%ld\n",strlen(buff));
        n=write(sockfd, buff, strlen(buff));
        printf("envie %d bytes al servidor!\n",n);
        bzero(buff, sizeof(buff));
        n = read(sockfd, buff, sizeof(buff));
        printf("Lei %d bytes From Server : [%s]",n,buff);


        break;
    }
    */
    printf("5\n");
}

/*
 *----------------------------------------------------------------------
 *
 * MakeHeader --
 *
 *      Constructs an FCGI_Header struct.
 *
 *----------------------------------------------------------------------
 */
FCGI_Header MakeHeader(
        int type,
        int requestId,
        int contentLength,
        int paddingLength)
{
    FCGI_Header header;
    header.version = FCGI_VERSION_1;
    header.type             = (unsigned char) type;
    header.requestIdB1      = (unsigned char) ((requestId     >> 8) & 0xff);
    header.requestIdB0      = (unsigned char) ((requestId         ) & 0xff);
    header.contentLengthB1  = (unsigned char) ((contentLength >> 8) & 0xff);
    header.contentLengthB0  = (unsigned char) ((contentLength     ) & 0xff);
    header.paddingLength    = (unsigned char) paddingLength;
    header.reserved         =  0;
    return header;
}


/*
 *----------------------------------------------------------------------
 *
 * MakeBeginRequestBody --
 *
 *      Constructs an FCGI_BeginRequestBody record.
 *
 *----------------------------------------------------------------------
 */
FCGI_BeginRequestBody MakeBeginRequestBody(
        int role,
        int keepConnection)
{
    FCGI_BeginRequestBody body;
    body.roleB1 = (unsigned char) ((role >>  8) & 0xff);
    body.roleB0 = (unsigned char) (role         & 0xff);
    body.flags  = (unsigned char) ((keepConnection) ? FCGI_KEEP_CONN : 0);
    memset(body.reserved, 0, sizeof(body.reserved));
    return body;
}


/*
 *----------------------------------------------------------------------
 *
 * FCGIUtil_BuildNameValueHeader --
 *
 *      Builds a name-value pair header from the name length
 *      and the value length.  Stores the header into *headerBuffPtr,
 *      and stores the length of the header into *headerLenPtr.
 *
 * Side effects:
 *      Stores header's length (at most 8) into *headerLenPtr,
 *      and stores the header itself into
 *      headerBuffPtr[0 .. *headerLenPtr - 1].
 *
 *----------------------------------------------------------------------
 */
void FCGIUtil_BuildNameValueHeader(
        int nameLen,
        int valueLen,
        unsigned char *headerBuffPtr,
        int *headerLenPtr) {
    unsigned char *startHeaderBuffPtr = headerBuffPtr;

    ASSERT(nameLen >= 0);
    if (nameLen < 0x80) {
        *headerBuffPtr++ = (unsigned char) nameLen;
    } else {
        *headerBuffPtr++ = (unsigned char) ((nameLen >> 24) | 0x80);
        *headerBuffPtr++ = (unsigned char) (nameLen >> 16);
        *headerBuffPtr++ = (unsigned char) (nameLen >> 8);
        *headerBuffPtr++ = (unsigned char) nameLen;
    }
    ASSERT(valueLen >= 0);
    if (valueLen < 0x80) {
        *headerBuffPtr++ = (unsigned char) valueLen;
    } else {
        *headerBuffPtr++ = (unsigned char) ((valueLen >> 24) | 0x80);
        *headerBuffPtr++ = (unsigned char) (valueLen >> 16);
        *headerBuffPtr++ = (unsigned char) (valueLen >> 8);
        *headerBuffPtr++ = (unsigned char) valueLen;
    }
    *headerLenPtr = headerBuffPtr - startHeaderBuffPtr;
}

static void WriteStdinEof(void)
{
    static int stdin_eof_sent = 0;

    if (stdin_eof_sent)
    	return;

    *((FCGI_Header *)fromWS.stop) = MakeHeader(FCGI_STDIN, requestId, 0, 0);
    fromWS.stop += sizeof(FCGI_Header);
    stdin_eof_sent = 1;
}


/*
 *----------------------------------------------------------------------
 *
 * FCGIexit --
 *
 *      FCGIexit provides a single point of exit.  It's main use is for
 *      application debug when porting to other operating systems.
 *
 *----------------------------------------------------------------------
 */
static void FCGIexit(int sockfd,int exitCode)
{
    if(sockfd != -1) {
        OS_Close(sockfd, TRUE);
	//appServerSock = -1;
    }
    OS_LibShutdown();
    exit(exitCode);
}

/*
 * ScheduleIo --
 *
 *      This functions is responsible for scheduling all I/O to move
 *      data between a web server and a FastCGI application.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      This routine will signal the ioEvent upon completion.
 *
 */
void ScheduleIo(int appServerSock)
{
printf("ScheduleIo inicio\n");
    int length;

    /*
     * Move data between standard in and the FastCGI connection.
     */
    if(!fcgiWritePending && appServerSock != -1 &&
       ((length = fromWS.stop - fromWS.next) != 0)) {
printf("ScheduleIo 0.5 appServerSock=%d length=%d fromWS.next=[%s]\n",appServerSock,length,fromWS.next);
			if(OS_AsyncWrite(appServerSock, 0, fromWS.next, length,
					 AppServerWriteHandler,
					 (ClientData)(intptr_t)appServerSock) == -1) {
				FCGIexit(appServerSock,OS_Errno);
			} else {
				fcgiWritePending = TRUE;
			}
    }
printf("ScheduleIo 1\n");

    /*
     * Schedule a read from the FastCGI application if there's not
     * one pending and there's room in the buffer.
     */
    if(!fcgiReadPending && appServerSock != -1) {
		fromAS.next = &fromAS.buff[0];
printf("ScheduleIo 1.1\n");
		if(OS_AsyncRead(appServerSock, 0, fromAS.next, BUFFLEN,
				AppServerReadHandler,
				(ClientData)(intptr_t)appServerSock) == -1) {
			FCGIexit(appServerSock,OS_Errno);
		} else {
			fcgiReadPending = TRUE;
		}
    }

printf("ScheduleIo 2\n");

    /*
     * Schedule a read from standard in if necessary.
     */
    if((bytesToRead > 0) && !webServerReadHandlerEOF && !wsReadPending &&
       !fcgiWritePending &&
       fromWS.next == &fromWS.buff[0]) {
printf("ScheduleIo 2.1\n");
		   
		if(OS_AsyncReadStdin(fromWS.next + sizeof(FCGI_Header),
					 BUFFLEN - sizeof(FCGI_Header),
					 WebServerReadHandler, STDIN_FILENO)== -1) {
			FCGIexit(appServerSock,OS_Errno);
		} else {
			wsReadPending = TRUE;
		}
    }
printf("ScheduleIo fin\n");

}


/*
 *----------------------------------------------------------------------
 *
 * AppServerWriteHandler --
 *
 *      Non-blocking writes data from the fromWS buffer to the FCGI
 *      application server.  Called only when fromWS is non-empty
 *      and the socket is ready to accept some data.
 *
 *----------------------------------------------------------------------
 */

static void AppServerWriteHandler(ClientData dc, int bytesWritten)
{
    int length = fromWS.stop - fromWS.next;

    /* Touch unused parameters to avoid warnings */
    dc = NULL;

    assert(length > 0);
    assert(fcgiWritePending == TRUE);

    fcgiWritePending = FALSE;
    if(bytesWritten < 0) {
        exit(OS_Errno);
    }
    if((int)bytesWritten < length) {
        fromWS.next += bytesWritten;
    } else {
        fromWS.stop = fromWS.next = &fromWS.buff[0];
    }

    ScheduleIo(fdServerSoc);
}

/*
 *----------------------------------------------------------------------
 *
 * AppServerReadHandler --
 *
 *      Reads data from the FCGI application server and (blocking)
 *      writes all of it to the Web server.  Exits the program upon
 *      reading EOF from the FCGI application server.  Called only when
 *      there's data ready to read from the application server.
 *
 *----------------------------------------------------------------------
 */

static void AppServerReadHandler(ClientData dc, int bytesRead)
{
    int count, outFD;
    char *ptr;

    /* Touch unused parameters to avoid warnings */
    dc = NULL;

    assert(fcgiReadPending == TRUE);
    fcgiReadPending = FALSE;
    count = bytesRead;

    if(count <= 0) {
        if(count < 0) {
            exit(OS_Errno);
        }
        if(headerLen > 0 || paddingLen > 0) {
            exit(FCGX_PROTOCOL_ERROR);
        }
	if(fdServerSoc != -1) {
	    OS_Close(fdServerSoc, TRUE);
	    fdServerSoc = -1;
	}
        /*
         * XXX: Shouldn't be here if exitStatusSet.
         */
        exit((exitStatusSet) ? exitStatus : FCGX_PROTOCOL_ERROR);
    }
    fromAS.stop = fromAS.next + count;
    while(fromAS.next != fromAS.stop) {
        /*
         * fromAS is not empty.  What to do with the contents?
         */
        if(headerLen < sizeof(header)) {
            /*
             * First priority is to complete the header.
             */
            count = GetPtr(&ptr, sizeof(header) - headerLen, &fromAS);
            assert(count > 0);
            memcpy(&header + headerLen, ptr, count);
            headerLen += count;
            if(headerLen < sizeof(header)) {
                break;
            }
            if(header.version != FCGI_VERSION_1) {
                exit(FCGX_UNSUPPORTED_VERSION);
	    }
            if((header.requestIdB1 << 8) + header.requestIdB0 != requestId) {
                exit(FCGX_PROTOCOL_ERROR);
	    }
            contentLen = (header.contentLengthB1 << 8)
                         + header.contentLengthB0;
            paddingLen =  header.paddingLength;
	} else {
            /*
             * Header is complete (possibly from previous call).  What now?
             */
            switch(header.type) {
	        case FCGI_STDOUT:
                case FCGI_STDERR:
                    /*
                     * Write the buffered content to stdout or stderr.
                     * Blocking writes are OK here; can't prevent a slow
                     * client from tying up the app server without buffering
                     * output in temporary files.
                     */
                    count = GetPtr(&ptr, contentLen, &fromAS);
                    contentLen -= count;
                    if(count > 0) {
                        outFD = (header.type == FCGI_STDOUT) ?
                                    STDOUT_FILENO : STDERR_FILENO;
                        if(OS_Write(outFD, ptr, count) < 0) {
                            exit(OS_Errno);
                        }
	            }
                    break;
                case FCGI_END_REQUEST:
                    if(!readingEndRequestBody) {
                        if(contentLen != sizeof(erBody)) {
                            exit(FCGX_PROTOCOL_ERROR);
		        }
                        readingEndRequestBody = TRUE;
		    }
                    count = GetPtr(&ptr, contentLen, &fromAS);
                    if(count > 0) {
                        memcpy(&erBody + sizeof(erBody) - contentLen,
                                ptr, count);
                        contentLen -= count;
		    }
                    if(contentLen == 0) {
                        if(erBody.protocolStatus != FCGI_REQUEST_COMPLETE) {
                            /*
                             * XXX: What to do with FCGI_OVERLOADED?
                             */
                            exit(FCGX_PROTOCOL_ERROR);
			}
                        exitStatus = (erBody.appStatusB3 << 24)
                                   + (erBody.appStatusB2 << 16)
                                   + (erBody.appStatusB1 <<  8)
                                   + (erBody.appStatusB0      );
                        exitStatusSet = TRUE;
                        readingEndRequestBody = FALSE;
	            }
                    break;
                case FCGI_GET_VALUES_RESULT:
                    /* coming soon */
                case FCGI_UNKNOWN_TYPE:
                    /* coming soon */
                default:
                    exit(FCGX_PROTOCOL_ERROR);
	    }
            if(contentLen == 0) {
                if(paddingLen > 0) {
                    paddingLen -= GetPtr(&ptr, paddingLen, &fromAS);
		}
                /*
                 * If we've processed all the data and skipped all the
                 * padding, discard the header and look for the next one.
                 */
                if(paddingLen == 0) {
                    headerLen = 0;
	        }
	    }
        } /* headerLen >= sizeof(header) */
    } /*while*/
    ScheduleIo(fdServerSoc);
}

/*
 *----------------------------------------------------------------------
 *
 * WebServerReadHandler --
 *
 *      Non-blocking reads data from the Web server into the fromWS
 *      buffer.  Called only when fromWS is empty, no EOF has been
 *      received from the Web server, and there's data available to read.
 *
 *----------------------------------------------------------------------
 */

static void WebServerReadHandler(ClientData dc, int bytesRead)
{
    /* Touch unused parameters to avoid warnings */
    dc = NULL;

    assert(fromWS.next == fromWS.stop);
    assert(fromWS.next == &fromWS.buff[0]);
    assert(wsReadPending == TRUE);
    wsReadPending = FALSE;

    if(bytesRead < 0) {
        exit(OS_Errno);
    }
    FCGI_Header *header = (FCGI_Header *) &fromWS.buff[0];
    *header = MakeHeader(FCGI_STDIN, requestId, bytesRead, 0);
    bytesToRead -= bytesRead;
    fromWS.stop = &fromWS.buff[sizeof(FCGI_Header) + bytesRead];
    webServerReadHandlerEOF = (bytesRead == 0);

    if (bytesToRead <= 0)
	WriteStdinEof();

    ScheduleIo(fdServerSoc);
}

/*
 *----------------------------------------------------------------------
 *
 * GetPtr --
 *
 *      Returns a count of the number of characters available
 *      in the buffer (at most n) and advances past these
 *      characters.  Stores a pointer to the first of these
 *      characters in *ptr.
 *
 *----------------------------------------------------------------------
 */

static int GetPtr(char **ptr, int n, Buffer *pBuf)
{
    int result;
    *ptr = pBuf->next;
    result = min(n, pBuf->stop - pBuf->next);
    pBuf->next += result;
    return result;
}

