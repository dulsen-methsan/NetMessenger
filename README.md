# NetMessenger

## IE3010 Network Programming Assignment

### Student Details

- Registration Number: IT23619944
- Node ID (NID): 6199

## Personalised Configuration

| Item | Value |
|---|---|
| Server Port | 15944 |
| Server Source File | server_9944.c |
| Client Source File | client_9944.c |
| Makefile | Makefile_9944 |
| Log File | netmsg_IT23619944.log |
| Storage Path | ./storage/IT23619944/ |
| Submission Archive | IE3010_IT23619944.zip |

### Port Calculation

Registration number: IT23619944

Last four digits: 9944

Server port:

6000 + 9944 = 15944

### NID Calculation

Numeric part of the registration number:

23619944

Middle four digits (digits 3–6):

6199

Therefore:

NID: 6199

## Project Description

NetMessenger is a multi-client TCP/IP chat and file-sharing application implemented in C using the standard BSD sockets API.

The system consists of:

- One TCP server
- Multiple TCP clients
- pthread-based concurrency
- User registration and presence notifications
- Broadcast messaging
- Private messaging
- Chat rooms
- File sharing
- Server-side logging
- Error handling
- Graceful and ungraceful disconnect handling

## Implemented Commands

The following commands are implemented:

```text
REGISTER <username>
LIST
BCAST <message>
PMSG <username> <message>
JOIN <room>
LEAVE <room>
ROOMS
RMSG <room> <message>
SENDFILE <target> <filename>
QUIT
