/*
ArpoiseDirectory.c - main for Arpoise Directory front end service.

Copyright (C) 2018, Tamiko Thiel and Peter Graf - All Rights Reserved

ARPOISE - Augmented Reality Point Of Interest Service

This file is part of Arpoise.

	Arpoise is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Arpoise is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Arpoise.  If not, see <https://www.gnu.org/licenses/>.

For more information on

Tamiko Thiel, see www.TamikoThiel.com/
Peter Graf, see www.mission-base.com/peter/
Arpoise, see www.Arpoise.com/

$Log: ArpoiseDirectory.c,v $
Revision 1.43  2020/03/18 22:37:22  peter
handle inner layer request for a default layer

Revision 1.41  2020/03/18 00:23:26  peter
Cleanup of getAreaConfigValue()

Revision 1.40  2020/03/17 22:11:00  peter
No caching

Revision 1.39  2020/03/17 22:01:30  peter
Made sure the directory server does not cache

Revision 1.38  2020/03/17 20:50:03  peter
Working on hidden histories version

Revision 1.37  2020/03/16 11:43:13  peter
Working on version for hidden histories

Revision 1.36  2020/03/15 23:29:25  peter
Working on areas for the directory

Revision 1.35  2020/02/23 12:29:21  peter
Added layer name to location statistics

Revision 1.34  2020/01/10 11:06:06  peter
Layer lists for AR-vos also for Android

Revision 1.33  2019/11/15 21:50:03  peter
Layer lists for AR-vos on iOs

Revision 1.32  2019/11/15 19:32:26  peter
Allowing the menu button in AR-vos

Revision 1.31  2019/10/07 14:46:18  peter
Allowing menu button for default layers

Revision 1.30  2019/09/02 20:29:34  peter
Some cleanup

Revision 1.29  2019/08/13 21:23:02  peter
Removed potential null reference crash.

Revision 1.28  2019/08/10 19:31:55  peter
Added Arslam handling

Revision 1.27  2019/07/23 21:54:34  peter
Separated version and location logs

Revision 1.26  2019/05/18 18:31:13  peter
Special default layer for AB EoF

Revision 1.25  2019/04/04 21:54:53  peter
Added the default layer handling for Arvos

Revision 1.24  2019/02/22 20:28:01  peter
Removed a compiler warning

Revision 1.23  2019/02/22 19:06:22  peter
Added the handling of the DefaultLayerName

Revision 1.22  2019/02/22 15:04:21  peter
Handling of showMenuButton for default layer

Revision 1.21  2019/02/18 20:22:18  peter
Directory requests with one hotspot as result are not sent to the client as list anymore

Revision 1.20  2019/02/17 17:07:12  peter
Handling of clients that can handle arpoise directory responses

Revision 1.19  2019/02/05 13:29:21  peter
Code simplyfication and refactoring

Revision 1.18  2019/02/05 11:14:22  peter
Refactoring of networking code

Revision 1.17  2019/02/04 19:44:53  peter
Improved the device position handling with the default layer

Revision 1.16  2019/02/04 12:29:03  peter
Improved the version statistics

Revision 1.15  2019/02/04 12:00:03  peter
Improved default layer handling

Revision 1.14  2019/02/03 23:22:28  peter
Some cleanup

Revision 1.13  2019/02/03 23:19:02  peter
Changed the directory structure of the hit count mechanism

Revision 1.12  2019/02/03 17:00:02  peter
Improved handling of redirections

Revision 1.11  2019/02/03 15:20:53  peter
Moved redirection to the client

Revision 1.10  2019/02/03 13:05:47  peter
Improved bundle handling

Revision 1.9  2019/02/03 12:52:47  peter
Fixed the count handling

Revision 1.8  2019/02/03 12:47:08  peter
Do not create statistics hits for refreshes

Revision 1.7  2019/02/03 12:03:06  peter
Creating the versions and layer names files for the hits

Revision 1.6  2019/02/03 01:30:41  peter
Versions file handling

Revision 1.5  2019/02/02 16:53:49  peter
Default layer is reign of gold

Revision 1.4  2019/01/30 23:45:24  peter
Fixed a bug with longer responses

Revision 1.3  2019/01/20 16:13:33  peter
Cleanup of traces

Revision 1.2  2019/01/19 15:50:38  peter
Improved the user agent string

Revision 1.1  2019/01/19 00:03:31  peter
Working on arpoise directory service


*/

/*
* Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
*/
char* ArpoiseDirectory_c_id = "$Id: ArpoiseDirectory.c,v 1.43 2020/03/18 22:37:22 peter Exp $";

#include <stdio.h>
#include <memory.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <assert.h>
#include <stdlib.h>

#ifdef _WIN32

#include <winsock2.h>
#include <direct.h>
#include <windows.h> 
#include <process.h>

#define socket_close closesocket

#else

#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define socket_close close

#ifndef h_addr
#define h_addr h_addr_list[0] /* for backward compatibility */
#endif

#endif

#include "pblCgi.h"

/*
 * Receive some bytes from a socket
 */
static int receiveBytesFromTcp(int socket, char* buffer, int bufferSize, struct timeval* timeout)
{
	char* tag = "readTcp";
	int    rc = 0;
	int    socketError = 0;
	unsigned int optlen = sizeof(socketError);

	errno = 0;
	if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&socketError, &optlen))
	{
		pblCgiExitOnError("%s: getsockopt(%d) error, errno %d\n", tag, socket, errno);
	}

	int nBytesRead = 0;
	while (nBytesRead < bufferSize)
	{
		fd_set readFds;
		FD_ZERO(&readFds);
		FD_SET(socket, &readFds);

		errno = 0;
		rc = select(socket + 1, &readFds, (fd_set*)NULL, (fd_set*)NULL, timeout);
		switch (rc)
		{
		case 0:
			return (-1);

		case -1:
			if (errno == EINTR)
			{
				pblCgiExitOnError("%s: select(%d) EINTR error, errno %d\n", tag, socket, errno);
			}
			pblCgiExitOnError("%s: select(%d) error, errno %d\n", tag, socket, errno);
			break;

		default:
			errno = 0;
			if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&socketError, &optlen))
			{
				pblCgiExitOnError("%s: getsockopt(%d) error, errno %d\n", tag, socket, errno);
			}

			if (socketError)
			{
				continue;
			}

			errno = 0;
			rc = recvfrom(socket, buffer + nBytesRead, bufferSize - nBytesRead, 0, NULL, NULL);
			if (rc < 0)
			{
				if (errno == EINTR)
				{
					pblCgiExitOnError("%s: recvfrom(%d) EINTR error, errno %d\n", tag, socket, errno);
				}
				pblCgiExitOnError("%s: recvfrom(%d) error, errno %d\n", tag, socket, errno);
			}
			else if (rc == 0)
			{
				return nBytesRead;
			}
			nBytesRead += rc;
		}
	}
	return nBytesRead;
}

static char receiveBuffer[64 * 1024];
/*
* Receive some string bytes and return the result in a malloced buffer.
*/
static char* receiveStringFromTcp(int socket, int timeoutSeconds)
{
	static char* tag = "receiveStringFromTcp";

	char* result = NULL;
	PblStringBuilder* stringBuilder = NULL;

	struct timeval timeoutValue;
	timeoutValue.tv_sec = timeoutSeconds;
	timeoutValue.tv_usec = 0;

	receiveBuffer[0] = '\0';

	for (;;)
	{
		int rc = receiveBytesFromTcp(socket, receiveBuffer, sizeof(receiveBuffer) - 1, &timeoutValue);
		if (rc < 0)
		{
			return NULL;
		}
		if (rc < 0 || rc > sizeof(receiveBuffer) - 1)
		{
			pblCgiExitOnError("%s: readTcp failed! rc %d\n", tag, rc);
		}
		else if (rc == 0)
		{
			break;
		}
		else
		{
			receiveBuffer[rc] = '\0';
		}

		if (rc < sizeof(receiveBuffer) - 1 && stringBuilder == NULL)
		{
			result = pblCgiStrDup(receiveBuffer);
			break;
		}

		if (stringBuilder == NULL)
		{
			stringBuilder = pblStringBuilderNew();
			if (!stringBuilder)
			{
				pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
		}
		if (pblStringBuilderAppendStr(stringBuilder, receiveBuffer) == ((size_t)-1))
		{
			pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}

	if (result == NULL)
	{
		if (stringBuilder == NULL)
		{
			pblCgiExitOnError("%s: socket %d received 0 bytes as response\n", tag, socket);
		}

		result = pblStringBuilderToString(stringBuilder);
		if (!result)
		{
			pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}
	if (stringBuilder)
	{
		pblStringBuilderFree(stringBuilder);
	}
	return result;
}

/*
* Send some bytes to a tcp socket
*/
static void sendBytesToTcp(int socket, char* buffer, int nBytesToSend)
{
	static char* tag = "sendBytesToTcp";

	char* ptr = buffer;
	while (nBytesToSend > 0)
	{
		errno = 0;
		int rc = send(socket, ptr, nBytesToSend, 0);
		if (rc > 0)
		{
			ptr += rc;
			nBytesToSend -= rc;
		}
		else
		{
			pblCgiExitOnError("%s: send(%d) error, rc %d, errno %d\n", tag, socket, rc, errno);
		}
	}
}

/*
* Connect to a tcp socket on machine with hostname and port
*/
static int connectToTcp(char* hostname, int port)
{
	static char* tag = "connectToTcp";

	errno = 0;
	struct hostent* hostInfo = gethostbyname(hostname);
	if (!hostInfo)
	{
		pblCgiExitOnError("%s: gethostbyname(%s) error, errno %d.\n", tag, hostname, errno);
		return -1;
	}

	short shortPort = 80;
	if (port > 0)
	{
		shortPort = port;
	}

	struct sockaddr_in serverAddress;
	memset((char*)&serverAddress, 0, sizeof(struct sockaddr_in));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(shortPort);
	memcpy(&(serverAddress.sin_addr.s_addr), hostInfo->h_addr, sizeof(serverAddress.sin_addr.s_addr));

	errno = 0;
	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
	{
		pblCgiExitOnError("%s: socket() error, errno %d\n", tag, errno);
	}

	errno = 0;
	if (connect(socketFd, (struct sockaddr*) & serverAddress, sizeof(struct sockaddr_in)) < 0)
	{
		pblCgiExitOnError("%s: connect(%d) error, host '%s' on port %d, errno %d\n", tag, socketFd, hostname, shortPort, errno);
		socket_close(socketFd);
	}
	return socketFd;
}

/*
* Make a HTTP request with the given uri to the given host/port
* and return the result content in a malloced buffer.
*/
static char* getHttpResponse(char* hostname, int port, char* uri, int timeoutSeconds, char* agent)
{
	char* response = NULL;
	for (int n = 0; n < 2; n++)
	{
		int socketFd = connectToTcp(hostname, port);

		char* sendBuffer = pblCgiSprintf("GET %s HTTP/1.0\r\nUser-Agent: %s\r\nHost: %s\r\n\r\n", uri, agent, hostname);
		PBL_CGI_TRACE("HttpRequest=%s", sendBuffer);

		sendBytesToTcp(socketFd, sendBuffer, strlen(sendBuffer));
		PBL_FREE(sendBuffer);

		response = receiveStringFromTcp(socketFd, timeoutSeconds);
		socket_close(socketFd);
		if (!response)
		{
			PBL_CGI_TRACE("HttpResponse=NULL, n=%d", n);
			continue;
		}
		PBL_CGI_TRACE("HttpResponse=%s", response);
		break;
	}
	if (!response)
	{
		pblCgiExitOnError("getHttpResponse: receiveStringFromTcp returned NULL\n");
	}
	return response;
}

static char* getMatchingString(char* string, char start, char end, char** nextPtr)
{
	char* tag = "getMatchingString";
	char* ptr = string;
	if (start != *ptr)
	{
		pblCgiExitOnError("%s: expected %c at start of string '%s'\n", tag, start, string);
	}

	int level = 1;
	int c;
	while ((c = *++ptr))
	{
		if (c == start)
		{
			level++;
		}
		if (c == end)
		{
			level--;
			if (level < 1)
			{
				if (nextPtr)
				{
					*nextPtr = ptr + 1;
				}
				return pblCgiStrRangeDup(string + 1, ptr);
			}
		}
	}
	pblCgiExitOnError("%s: unexpected end of string in '%s'\n", tag, string);
	return NULL;
}

static char* getStringBetween(char* string, char* start, char* end)
{
	char* tag = "getStringBetween";
	char* ptr = *start ? strstr(string, start) : string;
	if (!ptr)
	{
		pblCgiExitOnError("%s: expected starting tag '%s' in string '%s'\n", tag, start, string);
		return NULL;
	}
	ptr += strlen(start);
	char* ptr2 = strstr(ptr, end);
	if (!ptr2)
	{
		pblCgiExitOnError("%s: expected ending '%s' in string '%s'\n", tag, end, ptr);
	}
	return pblCgiStrRangeDup(ptr, ptr2);
}

static char* getNumberString(char* string, char* start)
{
	char* tag = "getNumberString";
	char* ptr = strstr(string, start);
	if (!ptr)
	{
		pblCgiExitOnError("%s: expected starting tag '%s' in string '%s'\n", tag, start, string);
	}
	ptr += strlen(start);

	char* ptr2 = ptr;
	while (ptr2 && *ptr2)
	{
		if (isdigit(*ptr2) || '.' == *ptr2 || '-' == *ptr2 || '+' == *ptr2)
		{
			ptr2++;
			continue;
		}
		break;
	}
	if (!ptr2)
	{
		pblCgiExitOnError("%s: expected number ending in string '%s'\n", tag, ptr);
	}
	return pblCgiStrRangeDup(ptr, ptr2);
}

static char* getHttpResponseBody(char* response, char** cookiePtr)
{
	static char* tag = "getHttpResponseBody";

	// check for HTTP error code like HTTP/1.1 500 Server Error
	//
	if (cookiePtr && strstr(response, "Set-Cookie: "))
	{
		*cookiePtr = getStringBetween(response, "Set-Cookie: ", "\r\n");
	}

	char* ptr = strstr(response, "HTTP/");
	if (ptr)
	{
		ptr = strstr(ptr, " ");
		if (ptr)
		{
			ptr++;
			if (strncmp(ptr, "200", 3))
			{
				pblCgiExitOnError("%s: Bad HTTP response\n%s\n", tag, response);
			}
		}
	}

	if (ptr)
	{
		char* end = strstr(ptr, "\r\n\r\n");
		if (!end)
		{
			end = strstr(ptr, "\n\n");
			if (!end)
			{
				pblCgiExitOnError("%s: Illegal HTTP response, no separator.\n%s\n", tag, response);
			}
			else
			{
				end += 2;
			}
		}
		else
		{
			end += 4;
		}
		return end;
	}

	pblCgiExitOnError("%s: Expecting HTTP response\n%s\n", tag, response);
	return NULL;
}

static void putString(char* string, PblStringBuilder* stringBuilder)
{
	char* tag = "putString";

	if (pblStringBuilderAppendStr(stringBuilder, string) == ((size_t)-1))
	{
		pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	fputs(string, stdout);
}

static char* changeLat(char* string, int i, int difference)
{
	if (!strstr(string, "\"lat\":"))
	{
		return pblCgiStrDup(string);
	}

	int factor = 1 + (i - 1) / 8;
	int modulo = (i - 1) % 8;

	switch (modulo)
	{
	case 0:
		difference *= factor;
		break;
	case 1:
		difference *= -factor;
		break;
	case 4:
	case 5:
		difference *= factor;
		break;
	case 6:
	case 7:
		difference *= -factor;
		break;
	default:
		return pblCgiStrDup(string);
	}

	char* lat = getNumberString(string, "\"lat\":");
	//PBL_CGI_TRACE("lat=%s", lat);

	char* oldLat = pblCgiSprintf("\"lat\":%s,", lat);
	//PBL_CGI_TRACE("oldLat=%s", oldLat);

	char* newLat = pblCgiSprintf("\"lat\":%d,", atoi(lat) + difference);
	//PBL_CGI_TRACE("newLat=%s", newLat);

	char* replacedLat = pblCgiStrReplace(string, oldLat, newLat);

	PBL_FREE(lat);
	PBL_FREE(oldLat);
	PBL_FREE(newLat);

	return replacedLat;
}

static char* changeLon(char* string, int i, int difference)
{
	if (!strstr(string, "\"lon\":"))
	{
		return pblCgiStrDup(string);
	}

	int factor = 1 + (i - 1) / 8;
	int modulo = (i - 1) % 8;

	switch (modulo)
	{
	case 2:
		difference *= factor;
		break;
	case 3:
		difference *= -factor;
		break;
	case 4:
	case 6:
		difference *= factor;
		break;
	case 5:
	case 7:
		difference *= -factor;
		break;
	default:
		return pblCgiStrDup(string);
	}

	char* lon = getNumberString(string, "\"lon\":");
	//PBL_CGI_TRACE("lon=%s", lon);

	char* oldLon = pblCgiSprintf("\"lon\":%s,", lon);
	//PBL_CGI_TRACE("oldLon=%s", oldLon);

	char* newLon = pblCgiSprintf("\"lon\":%d,", atoi(lon) + difference);
	//PBL_CGI_TRACE("newLon=%s", newLon);

	char* replacedLon = pblCgiStrReplace(string, oldLon, newLon);

	PBL_FREE(lon);
	PBL_FREE(oldLon);
	PBL_FREE(newLon);

	return replacedLon;
}

static char* changeLatAndLon(char* queryString, char* lat, char* lon, int* latDifference, int* lonDifference)
{
	if (!pblCgiStrIsNullOrWhiteSpace(lat) && !pblCgiStrIsNullOrWhiteSpace(lon))
	{
		char* replacementLat = pblCgiStrCat("lat=", lat);
		char* replacementLon = pblCgiStrCat("lon=", lon);
		double latDouble = strtod(replacementLat + 4, NULL);
		int replacementLatInteger = (int)(1000000.0 * latDouble);
		double lonDouble = strtod(replacementLon + 4, NULL);
		int replacementLonInteger = (int)(1000000.0 * lonDouble);
		int latPtrInteger = 0;
		int lonPtrInteger = 0;

		char* latPtr = strstr(queryString, "lat=");
		if (latPtr)
		{
			char* ptr = strstr(latPtr, "&");
			if (ptr)
			{
				latPtr = pblCgiStrRangeDup(latPtr, ptr);
			}
			else
			{
				latPtr = pblCgiStrDup(latPtr);
			}
			latPtrInteger = (int)(1000000.0 * strtod(latPtr + 4, NULL));
			queryString = pblCgiStrReplace(queryString, latPtr, replacementLat);
		}
		char* lonPtr = strstr(queryString, "lon=");
		if (lonPtr)
		{
			char* ptr = strstr(lonPtr, "&");
			if (ptr)
			{
				lonPtr = pblCgiStrRangeDup(lonPtr, ptr);
			}
			else
			{
				lonPtr = pblCgiStrDup(lonPtr);
			}
			lonPtrInteger = (int)(1000000.0 * strtod(lonPtr + 4, NULL));
			queryString = pblCgiStrReplace(queryString, lonPtr, replacementLon);
		}
		if (latDifference && lonDifference && latPtrInteger != 0 && lonPtrInteger != 0)
		{
			*latDifference = replacementLatInteger - latPtrInteger;
			*lonDifference = replacementLonInteger - lonPtrInteger;
		}
		return queryString;
	}
	return NULL;
}

static char* changeRedirectionUrl(char* string, char* redirectionUrl)
{
	if (!strstr(string, "\"redirectionUrl\":"))
	{
		return pblCgiStrDup(string);
	}

	char* oldValue = getStringBetween(string, "\"redirectionUrl\":", ",\"");

	char* oldValueStr = pblCgiSprintf("\"redirectionUrl\":%s", oldValue);
	char* newValueStr = pblCgiSprintf("\"redirectionUrl\":\"%s\"", redirectionUrl);

	char* replacedString = pblCgiStrReplace(string, oldValueStr, newValueStr);

	PBL_FREE(oldValue);
	PBL_FREE(oldValueStr);
	PBL_FREE(newValueStr);

	return replacedString;
}

static char* changeRedirectionLayer(char* string, char* redirectionLayer)
{
	if (!strstr(string, "\"redirectionLayer\":"))
	{
		return pblCgiStrDup(string);
	}

	char* oldValue = getStringBetween(string, "\"redirectionLayer\":", ",\"");

	char* oldValueStr = pblCgiSprintf("\"redirectionLayer\":%s", oldValue);
	char* newValueStr = pblCgiSprintf("\"redirectionLayer\":\"%s\"", redirectionLayer);

	char* replacedString = pblCgiStrReplace(string, oldValueStr, newValueStr);

	PBL_FREE(oldValue);
	PBL_FREE(oldValueStr);
	PBL_FREE(newValueStr);

	return replacedString;
}

static char* changeLayerName(char* string, char* layerName)
{
	if (!strstr(string, "layerName="))
	{
		return pblCgiStrDup(string);
	}

	char* oldLayerName = getStringBetween(string, "layerName=", "&");

	char* oldLayerNameStr = pblCgiSprintf("layerName=%s", oldLayerName);
	char* newLayerNameStr = pblCgiSprintf("layerName=%s", layerName);

	char* replacedString = pblCgiStrReplace(string, oldLayerNameStr, newLayerNameStr);

	PBL_FREE(oldLayerName);
	PBL_FREE(oldLayerNameStr);
	PBL_FREE(newLayerNameStr);

	return replacedString;
}

static char* changeShowMenuOption(char* string, char* value)
{
	if (!strstr(string, "\"showMenuButton\":"))
	{
		return pblCgiStrDup(string);
	}

	char* oldValue = getStringBetween(string, "\"showMenuButton\":", ",\"");

	char* oldValueStr = pblCgiSprintf("\"showMenuButton\":%s", oldValue);
	char* newValueStr = pblCgiSprintf("\"showMenuButton\":\"%s\"", value);

	char* replacedString = pblCgiStrReplace(string, oldValueStr, newValueStr);

	PBL_FREE(oldValue);
	PBL_FREE(oldValueStr);
	PBL_FREE(newValueStr);

	return replacedString;
}

static PblList* devicePositionList = NULL;

static char* handleDevicePosition(char* deviceId, char* client, char* queryString, int* latDifference, int* lonDifference)
{
	if (pblCgiStrIsNullOrWhiteSpace(deviceId))
	{
		return NULL;
	}

	char* lat = NULL;
	char* lon = NULL;

	if (!devicePositionList)
	{
		char* devicePositionValue = pblCgiConfigValue("DevicePosition", NULL);
		if (pblCgiStrIsNullOrWhiteSpace(devicePositionValue))
		{
			return NULL;
		}
		devicePositionList = pblCgiStrSplitToList(devicePositionValue, ",");
		if (pblListIsEmpty(devicePositionList))
		{
			PBL_CGI_TRACE("DevicePositionList is empty");
			return NULL;
		}
	}

	int listSize = pblListSize(devicePositionList);

	for (int i = 0; i < listSize - 2; i += 3)
	{
		char* device = pblListGet(devicePositionList, i);

		if (pblCgiStrEquals(deviceId, device))
		{
			lat = pblListGet(devicePositionList, i + 1);
			lon = pblListGet(devicePositionList, i + 2);
			break;
		}
	}
	return changeLatAndLon(queryString, lat, lon, latDifference, lonDifference);
}

static void traceDuration()
{
	struct timeval now;
	gettimeofday(&now, NULL);

	unsigned long duration = now.tv_sec * 1000000 + now.tv_usec;
	duration -= pblCgiStartTime.tv_sec * 1000000 + pblCgiStartTime.tv_usec;
	char* string = pblCgiSprintf("%lu", duration);
	PBL_CGI_TRACE("Duration=%s microseconds", string);
}

static void printHeader(char* cookie)
{
	fputs("Content-Type: application/json\r\n", stdout);
	if (cookie)
	{
		fputs("Set-Cookie: ", stdout);
		fputs(cookie, stdout);
		fputs("\r\n", stdout);
	}
	fputs("\r\n", stdout);
}

static void handleResponse(char* response, int latDifference, int lonDifference)
{
	static char* tag = "handleResponse";
	char* cookie = NULL;
	response = getHttpResponseBody(response, &cookie);

	char* start = "{\"hotspots\":";
	int length = strlen(start);

	if (strncmp(start, response, length))
	{
		printHeader(cookie);
		fputs(response, stdout);
		PBL_CGI_TRACE("Response does not start with %s, no handling", start);
		return;
	}

	PblStringBuilder* stringBuilder = pblStringBuilderNew();
	if (!stringBuilder)
	{
		pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	char* rest = NULL;
	char* hotspotsString = getMatchingString(response + length, '[', ']', &rest);

	PblList* list = pblListNewArrayList();
	if (!list)
	{
		pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	char* ptr = hotspotsString;
	while (*ptr == '{')
	{
		char* ptr2 = NULL;
		char* hotspot = getMatchingString(ptr, '{', '}', &ptr2);

		if (pblListAdd(list, hotspot) < 0)
		{
			pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		if (*ptr2 != ',')
		{
			break;
		}
		ptr = ptr2 + 1;
	}

	printHeader(cookie);
	putString(start, stringBuilder);
	putString("[", stringBuilder);

	int nPois = pblListSize(list);
	PBL_CGI_TRACE("Number of pois=%d", nPois);

	for (int j = 0; j < nPois; j++)
	{
		if (j > 0)
		{
			putString(",", stringBuilder);
		}
		putString("{", stringBuilder);

		char* hotspot = pblListGet(list, j);

		char* ptr = hotspot;
		if (latDifference != 0 || lonDifference != 0)
		{
			char* replacedLat = changeLat(hotspot, 1, -1 * latDifference);
			ptr = changeLon(replacedLat, 5, -1 * lonDifference);
			PBL_CGI_TRACE("Applied latDifference=%d and lonDifference=%d", latDifference, lonDifference);
		}
		putString(ptr, stringBuilder);
		putString("}", stringBuilder);
	}

	putString("]", stringBuilder);

	putString(rest, stringBuilder);
	PBL_CGI_TRACE("output=%s", pblStringBuilderToString(stringBuilder));
	pblStringBuilderFree(stringBuilder);
}

static void createStatisticsFile(char* directory, char* fileName)
{
	char* filePath = pblCgiSprintf("%s/%s", directory, fileName);

	FILE* stream = NULL;

#ifdef WIN32
	errno_t err = fopen_s(&stream, filePath, "r");
	if (err != 0)
	{
		stream = NULL;
	}
#else
	stream = fopen(filePath, "r");
#endif
	if (!stream)
	{
		stream = pblCgiFopen(filePath, "a");
		if (stream)
		{
			fputs("<title>Arpoise</title>\n<body>copyright � 2019, Tamiko Thiel and Peter Graf</body>\n", stream);
		}
	}
	if (stream)
	{
		fclose(stream);
	}
	PBL_FREE(filePath);
	}

static char* getVersion()
{
	return getStringBetween(ArpoiseDirectory_c_id, "ArpoiseDirectory.c,v ", " ");
}

static void createStatisticsHits(int layer, char* layerName, int layerServed)
{
	char* count = pblCgiQueryValue("count");
	if (pblCgiStrEquals("1", count))
	{
		PBL_CGI_TRACE("-------> Statistics Request\n");

		// Create a web hit for the os and bundle, so that web stats can be used to count hits

		char* versionsDirectory = pblCgiConfigValue("VersionsDirectory", "");
		if (versionsDirectory && *versionsDirectory)
		{
			char* os = pblCgiQueryValue("os");
			if (!os || !*os || strstr(os, ".."))
			{
				os = "UnknownOperatingSystem";
			}

			char* bundle = pblCgiQueryValue("bundle");
			if (!bundle || !*bundle || strstr(bundle, ".."))
			{
				bundle = "UnknownBundle";
			}

			char* fileName = pblCgiSprintf("%s_%s.htm", os, bundle);
			createStatisticsFile(versionsDirectory, fileName);
			char* uri = pblCgiSprintf("/ArpoiseDirectory/AppVersions/%s", fileName);
			getHttpResponse("www.arpoise.com", 80, uri, 16, "ArpoiseDirectory/AppVersions");
		}

		// Create a web hit for the location, so that web stats can be used to count hits

		char* locationsDirectory = pblCgiConfigValue("LocationsDirectory", "");
		if (locationsDirectory && *locationsDirectory)
		{
			char* queryLat = pblCgiQueryValue("lat");
			if (!queryLat || !*queryLat || strstr(queryLat, ".."))
			{
				queryLat = "UnknownLat";
			}
			else
			{
				queryLat = pblCgiStrDup(queryLat);
			}
			char* ptr = strstr(queryLat, ".");
			if (ptr && strlen(ptr) > 4)
			{
				ptr[4] = '\0'; // truncate latitude to 3 digits after the '.'
			}

			char* queryLon = pblCgiQueryValue("lon");
			if (!queryLon || !*queryLon || strstr(queryLon, ".."))
			{
				queryLon = "UnknownLon";
			}
			else
			{
				queryLon = pblCgiStrDup(queryLon);
			}
			ptr = strstr(queryLon, ".");
			if (ptr && strlen(ptr) > 4)
			{
				ptr[4] = '\0'; // truncate longitude to 3 digits after the '.'
			}

			char* fileName = pblCgiSprintf("%s_%s-%s.htm", queryLon, queryLat, layerName);
			createStatisticsFile(locationsDirectory, fileName);
			char* uri = pblCgiSprintf("/ArpoiseDirectory/Locations/%s", fileName);
			getHttpResponse("www.arpoise.com", 80, uri, 16, "ArpoiseDirectory/Locations");
		}

		// Create a web hit for the layer, so that web stats can be used to count hits

		char* layersDirectory = pblCgiConfigValue("LayersDirectory", "");
		if (layer && layersDirectory && *layersDirectory && layerName && *layerName)
		{
			if (!layerName || !*layerName || strstr(layerName, ".."))
			{
				layerName = "UnknownLayer";
			}

			char* fileName = pblCgiSprintf("%s.htm", layerName);
			createStatisticsFile(layersDirectory, fileName);
			char* uri = pblCgiSprintf("/ArpoiseDirectory/Layers/%s", fileName);
			getHttpResponse("www.arpoise.com", 80, uri, 16, "ArpoiseDirectory/Layers");
		}

		// Create a web hit for the layer served, so that web stats can be used to count hits

		char* layersServedDirectory = pblCgiConfigValue("LayersServedDirectory", "");
		if (layerServed && layersServedDirectory && *layersServedDirectory && layerName && *layerName)
		{
			if (!layerName || !*layerName || strstr(layerName, ".."))
			{
				layerName = "UnknownLayer";
			}

			char* fileName = pblCgiSprintf("%s.htm", layerName);
			createStatisticsFile(layersServedDirectory, fileName);
			char* uri = pblCgiSprintf("/ArpoiseDirectory/LayersServed/%s", fileName);
			getHttpResponse("www.arpoise.com", 80, uri, 16, "ArpoiseDirectory/LayersServed");
		}
	}
}

static void freeStringList(PblList* list)
{
	while (!pblListIsEmpty(list))
	{
		free(pblListPop(list));
	}
	pblListFree(list);
}

static char* getArea(char* queryString)
{
	int lat = 0;
	int lon = 0;
	char* latPtr = strstr(queryString, "lat=");
	if (latPtr)
	{
		char* ptr = strstr(latPtr, "&");
		if (ptr)
		{
			latPtr = pblCgiStrRangeDup(latPtr, ptr);
		}
		else
		{
			latPtr = pblCgiStrDup(latPtr);
		}
		double latDouble = strtod(latPtr + 4, NULL);
		lat = (int)(1000000.0 * latDouble);
	}
	char* lonPtr = strstr(queryString, "lon=");
	if (lonPtr)
	{
		char* ptr = strstr(lonPtr, "&");
		if (ptr)
		{
			lonPtr = pblCgiStrRangeDup(lonPtr, ptr);
		}
		else
		{
			lonPtr = pblCgiStrDup(lonPtr);
		}
		double lonDouble = strtod(lonPtr + 4, NULL);
		lon = (int)(1000000.0 * lonDouble);
	}
	for (int i = 1; i <= 1000; i++)
	{
		char* areaKey = pblCgiSprintf("Area_%d", i);
		char* areaValue = pblCgiConfigValue(areaKey, NULL);

		if (pblCgiStrIsNullOrWhiteSpace(areaValue))
		{
			PBL_CGI_TRACE("No value for area %s", areaKey);
			PBL_FREE(areaKey);
			return NULL;
		}

		PblList* locationList = pblCgiStrSplitToList(areaValue, ",");
		int size = pblListSize(locationList);
		if (size != 4)
		{
			PBL_CGI_TRACE("%s, expecting 4 location values, current value is %s", areaKey, areaValue);

			freeStringList(locationList);
			PBL_FREE(areaKey);
			continue;
		}

		int list0 = atoi(pblListGet(locationList, 0));
		int list1 = atoi(pblListGet(locationList, 1));
		int list2 = atoi(pblListGet(locationList, 2));
		int list3 = atoi(pblListGet(locationList, 3));

		if (lat < list0 || lon < list1 || lat > list2 || lon > list3)
		{
			PBL_CGI_TRACE("%s, lat %d, lon %d is outside area value %s", areaKey, lat, lon, areaValue);

			freeStringList(locationList);
			PBL_FREE(areaKey);
			continue;
		}
		PBL_CGI_TRACE("%s, lat %d, lon %d is inside area value %s", areaKey, lat, lon, areaValue);

		freeStringList(locationList);
		return areaKey;
	}
	return NULL;
}

static char* getAreaConfigValue(char* area, char* key, char* defaultValue)
{
	char* valueString = NULL;
	if (!pblCgiStrIsNullOrWhiteSpace(area))
	{
		char* areaKey = pblCgiSprintf("%s_%s", area, key);
		valueString = pblCgiConfigValue(areaKey, NULL);
		PBL_FREE(areaKey);
	}
	if (pblCgiStrIsNullOrWhiteSpace(valueString))
	{
		valueString = pblCgiConfigValue(key, defaultValue);
	}
	if (pblCgiStrIsNullOrWhiteSpace(valueString))
	{
		PBL_CGI_TRACE("No value for %s", key);
	}
	return valueString;
}

int showDefaultLayer = 1;

static int arpoiseDirectory(int argc, char* argv[])
{
	char* tag = "ArpoiseDirectory";
	int layerServed = 0;
	int layer = 0;

	struct timeval startTime;
	gettimeofday(&startTime, NULL);

#ifdef _WIN32

	pblCgiConfigMap = pblCgiFileToMap(NULL, "../config/Win32ArpoiseDirectory.txt");

#else

	pblCgiConfigMap = pblCgiFileToMap(NULL, "../config/ArpoiseDirectory.txt");

#endif

	char* traceFile = pblCgiConfigValue(PBL_CGI_TRACE_FILE, "/tmp/ArpoiseDirectory.txt");
	pblCgiInitTrace(&startTime, traceFile);
	PBL_CGI_TRACE("argc %d argv[0] = %s", argc, argv[0]);

	pblCgiParseQuery(argc, argv);
	char* queryString = pblCgiQueryString;

#ifdef _WIN32

	// Initialize Winsock
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		pblCgiExitOnError("%s: WSAStartup failed: %d\n", tag, result);
	}

#endif

	// read query values
	//
	char* client = pblCgiQueryValue("client");
	char* userId = pblCgiQueryValue("userId");
	if (!userId || !*userId)
	{
		userId = "UnknownUserId";
	}

	// handle fixed device positions
	//
	int latDifference = 0;
	int lonDifference = 0;
	char* deviceQueryString = handleDevicePosition(userId, client, queryString, &latDifference, &lonDifference);
	if (deviceQueryString != NULL)
	{
		queryString = deviceQueryString;
	}

	char* layerName = pblCgiQueryValue("layerName");
	char* layerUrl = "";
	char* uri = "";
	char* area = getArea(queryString);

	if (pblCgiStrEquals("true", pblCgiQueryValue("innerLayer"))
		&& pblCgiStrEquals("0.000000", pblCgiQueryValue("lat"))
		&& pblCgiStrEquals("0.000000", pblCgiQueryValue("lon")))
	{
		// an inner layer request for a default layer
		//
		queryString = pblCgiQueryString;
	}

	// Read config values
	//
	char* hostName = getAreaConfigValue(area, "HostName", "www.arpoise.com");
	if (pblCgiStrIsNullOrWhiteSpace(hostName))
	{
		pblCgiExitOnError("%s: HostName must be given.\n", tag);
	}
	PBL_CGI_TRACE("HostName=%s", hostName);

	int port = 80;
	char* portString = getAreaConfigValue(area, "Port", "80");
	if (!pblCgiStrIsNullOrWhiteSpace(portString))
	{
		int givenPort = atoi(portString);
		if (givenPort < 1)
		{
			pblCgiExitOnError("%s: Bad port %d.\n", tag, givenPort);
		}
		port = givenPort;
	}
	PBL_CGI_TRACE("Port=%d", port);

	char* directoryUri = getAreaConfigValue(area, "DirectoryUri", "/php/dir/web/porpoise.php");
	if (pblCgiStrIsNullOrWhiteSpace(directoryUri))
	{
		pblCgiExitOnError("%s: DirectoryUri must be given.\n", tag);
	}

	int isDirectoryRequest = pblCgiStrEquals(layerName, "Arpoise-Directory");
	if (isDirectoryRequest)
	{
		// See what operating system it is
		char* os = pblCgiQueryValue("os");
		if (!os || !*os)
		{
			os = "UnknownOperatingSystem";
		}

		int bundleInteger = 0;
		char* bundle = pblCgiQueryValue("bundle");
		if (bundle && isdigit(*bundle))
		{
			bundleInteger = atoi(bundle);
		}

		// This is a request for the Arpoise-Directory layer

		PBL_CGI_TRACE("-------> Directory Request\n");

		// See what client it is
		if (pblCgiStrEquals("Arvos", client))
		{
			if (pblCgiStrEquals("Android", os) && bundleInteger < 200101)
			{
				// Request the default layer from porpoise and return it to the client

				layerUrl = getAreaConfigValue(area, "ArvosDefaultLayerUrl", "/php/porpoise/web/porpoise.php");
				layerName = getAreaConfigValue(area, "ArvosDefaultLayerName", "Default-ImageTrigger");

				layerServed = 1;
				PBL_CGI_TRACE("-------> Arvos Default Layer Request: '%s' '%s'\n", layerUrl, layerName);

				char* ptr = changeLayerName(queryString, layerName);

				int myLatDifference = 0;
				int myLonDifference = 0;
				ptr = changeLatAndLon(ptr, "0.000000", "0.000000", &myLatDifference, &myLonDifference);
				latDifference += myLatDifference;
				lonDifference += myLonDifference;

				uri = pblCgiSprintf("%s?p=%d&%s", layerUrl, getpid(), ptr);
				char* agent = pblCgiSprintf("ArpoiseDirectory/%s", getVersion());
				char* response = getHttpResponse(hostName, port, uri, 16, agent);
				handleResponse(response, latDifference, lonDifference);

				createStatisticsHits(layer, layerName, layerServed);
				return 0;
			}

			// Request the AR-vos-Directory

			queryString = changeLayerName(queryString, "AR-vos-Directory");
		}
		if (pblCgiStrEquals("Arslam", client))
		{
			// Request the default layer from porpoise and return it to the client

			layerUrl = getAreaConfigValue(area, "ArslamDefaultLayerUrl", "/php/porpoise/web/porpoise.php");
			layerName = getAreaConfigValue(area, "ArslamDefaultLayerName", "Default-Slam");

			layerServed = 1;
			PBL_CGI_TRACE("-------> Arslam Default Layer Request: '%s' '%s'\n", layerUrl, layerName);

			char* ptr = changeLayerName(queryString, layerName);

			int myLatDifference = 0;
			int myLonDifference = 0;
			ptr = changeLatAndLon(ptr, "0.000000", "0.000000", &myLatDifference, &myLonDifference);
			latDifference += myLatDifference;
			lonDifference += myLonDifference;

			uri = pblCgiSprintf("%s?p=%d&%s", layerUrl, getpid(), ptr);
			char* agent = pblCgiSprintf("ArpoiseDirectory/%s", getVersion());
			char* response = getHttpResponse(hostName, port, uri, 16, agent);
			response = changeShowMenuOption(response, "false");
			handleResponse(response, latDifference, lonDifference);

			createStatisticsHits(layer, layerName, layerServed);
			return 0;
		}

		uri = pblCgiSprintf("%s?p=%d&%s", directoryUri, getpid(), queryString);
		char* cookie = NULL;

		char* httpResponse = getHttpResponse(hostName, port, uri, 16, pblCgiSprintf("ArpoiseClient %s", userId));
		char* response = getHttpResponseBody(httpResponse, &cookie);

		char* start = "{\"hotspots\":";
		int length = strlen(start);

		if (strncmp(start, response, length))
		{
			// There is nothing at the location the client is at

			if (!showDefaultLayer)
			{
				printHeader(cookie);
				fputs(response, stdout);
				PBL_CGI_TRACE("Response does not start with %s, no handling", start);
			}
			else if (pblCgiStrEquals("Arvos", client))
			{
				// Request the default layer from porpoise and return it to the client

				layerUrl = getAreaConfigValue(area, "ArvosDefaultLayerUrl", "/php/porpoise/web/porpoise.php");
				layerName = getAreaConfigValue(area, "ArvosDefaultLayerName", "Default-ImageTrigger");

				layerServed = 1;
				PBL_CGI_TRACE("-------> Arvos Default Layer Request: '%s' '%s'\n", layerUrl, layerName);

				char* ptr = changeLayerName(queryString, layerName);

				int myLatDifference = 0;
				int myLonDifference = 0;
				ptr = changeLatAndLon(ptr, "0.000000", "0.000000", &myLatDifference, &myLonDifference);
				latDifference += myLatDifference;
				lonDifference += myLonDifference;

				uri = pblCgiSprintf("%s?p=%d&%s", layerUrl, getpid(), ptr);
				char* agent = pblCgiSprintf("ArpoiseDirectory/%s", getVersion());
				char* response = getHttpResponse(hostName, port, uri, 16, agent);
				handleResponse(response, latDifference, lonDifference);

				createStatisticsHits(layer, layerName, layerServed);
				return 0;
			}
			else
			{
				// Request the default layer from porpoise and return it to the client

				layerUrl = getAreaConfigValue(area, "DefaultLayerUrl", "/php/porpoise/web/porpoise.php");
				layerName = getAreaConfigValue(area, "DefaultLayerName", "Default-Layer-Reign-of-Gold");

				if ((pblCgiStrEquals("Android", os) && bundleInteger >= 190310)
					|| (pblCgiStrEquals("iOS", os) && bundleInteger >= 20190310)
					)
				{
					char* layername190310 = getAreaConfigValue(area, "DefaultLayerName190310", "");
					if (layername190310 && *layername190310)
					{
						layerName = layername190310;
					}
				}

				layerServed = 1;
				PBL_CGI_TRACE("-------> Default Layer Request: '%s' '%s'\n", layerUrl, layerName);

				char* ptr = changeLayerName(queryString, layerName);

				int myLatDifference = 0;
				int myLonDifference = 0;
				ptr = changeLatAndLon(ptr, "0.000000", "0.000000", &myLatDifference, &myLonDifference);
				latDifference += myLatDifference;
				lonDifference += myLonDifference;

				uri = pblCgiSprintf("%s?p=%d&%s", layerUrl, getpid(), ptr);
				char* agent = pblCgiSprintf("ArpoiseDirectory/%s", getVersion());
				char* response = getHttpResponse(hostName, port, uri, 16, agent);
				//response = changeShowMenuOption(response, "false");
				handleResponse(response, latDifference, lonDifference);
			}
		}
		else
		{
			// There is at least one layer at the location the client is at

			// If there is more than one layer,
			// and the client can handle the response of the directory request,
			// send the response back to the client

			int numberOfHotspots = 0;
			char* numberOfHotspotsString = getStringBetween(response, "\"numberOfHotspots\":", ",\"");
			if (numberOfHotspotsString && isdigit(*numberOfHotspotsString))
			{
				numberOfHotspots = atoi(numberOfHotspotsString);
			}

			if (numberOfHotspots > 1
				&& (
				(
					pblCgiStrEquals("Android", os) && bundleInteger >= 190208)
					|| (pblCgiStrEquals("iOS", os) && bundleInteger >= 20190208)
					)
				)
			{
				PBL_CGI_TRACE("-------> Client response");

				handleResponse(httpResponse, latDifference, lonDifference);
			}
			else
			{
				char* baseUrlStart = "\"baseURL\":\"";
				char* ptr = strstr(response, baseUrlStart);
				if (ptr)
				{
					layerUrl = getStringBetween(ptr, baseUrlStart, "\"");
					while (strchr(layerUrl, '\\'))
					{
						layerUrl = pblCgiStrReplace(layerUrl, "\\", "");
					}
				}

				if (!layerUrl || !*layerUrl)
				{
					printHeader(cookie);
					fputs(response, stdout);
					PBL_CGI_TRACE("Response does not contain proper 'baseURL' value, no handling");
					return 0;
				}

				char* titleStart = "\"title\":\"";
				ptr = strstr(response, titleStart);
				if (ptr)
				{
					layerName = getStringBetween(ptr, titleStart, "\"");
				}

				if (!layerName || !*layerName)
				{
					printHeader(cookie);
					fputs(response, stdout);
					PBL_CGI_TRACE("Response does not contain proper 'title' value, no handling");
					return 0;
				}

				// Redirect the client to the url and layer specified

				layer = 1;
				ptr = changeRedirectionUrl(response, layerUrl);
				ptr = changeRedirectionLayer(ptr, layerName);

				printHeader(cookie);
				fputs(ptr, stdout);
				PBL_CGI_TRACE("-------> Client redirect: '%s' '%s'", layerUrl, layerName);
			}
		}
	}
	else
	{
		// This is a request for a specific layer, request the layer from porpoise and return it to the client
		char* porpoiseUri = getAreaConfigValue(area, "PorpoiseUri", "/php/porpoise/web/porpoise.php");
		if (pblCgiStrIsNullOrWhiteSpace(porpoiseUri))
		{
			pblCgiExitOnError("%s: PorpoiseUri must be given.\n", tag);
		}

		layerServed = 1;
		PBL_CGI_TRACE("-------> Layer Request: '%s' '%s'\n", porpoiseUri, layerName);

		uri = pblCgiSprintf("%s?p=%d&%s", porpoiseUri, getpid(), queryString);
		char* agent = pblCgiSprintf("ArpoiseFilter/%s", getVersion());
		handleResponse(getHttpResponse(hostName, port, uri, 16, agent), latDifference, lonDifference);
	}

	createStatisticsHits(layer, layerName, layerServed);
	return 0;
}

int main(int argc, char* argv[])
{
	int rc = arpoiseDirectory(argc, argv);
	traceDuration();
	return rc;
}
