# NetMessenger

## IE3010 - Network Programming

### NetMessenger: A Multi-Client Chat and File-Sharing Platform over TCP/IP

---

## 1. Student Details

- Registration Number: **IT23619944**
- Node ID (NID): **6199**
- Module: **IE3010 - Network Programming**
- Assignment: **NetMessenger**

---

## 2. Personalisation Details

The implementation uses the personalised values derived from the registration number according to the assignment specification.

| Item | Personalised Value |
|---|---|
| Registration Number | IT23619944 |
| Last Four Digits | 9944 |
| Middle Four Digits | 6199 |
| Server Port | 15944 |
| Server Source File | server_9944.c |
| Client Source File | client_9944.c |
| Makefile | Makefile_9944 |
| Node ID (NID) | 6199 |
| Log File | netmsg_IT23619944.log |
| Storage Root | ./storage/IT23619944/ |
| Submission Archive | IE3010_IT23619944.zip |

### Port Calculation

Registration number:

```text
IT23619944
```

Last four digits:

```text
9944
```

Server port calculation:

```text
6000 + 9944 = 15944
```

Therefore, the server listens on TCP port **15944**.

### NID Calculation

Numeric part of the registration number:

```text
23619944
```

The middle four digits (3rd to 6th digits) are:

```text
6199
```

Therefore:

```text
NID:6199
```

### Personalised File Names

```text
server_9944.c
client_9944.c
Makefile_9944
netmsg_IT23619944.log
```

### Personalised Storage Path

Received files are stored using:

```text
./storage/IT23619944/<sender_username>/<filename>
```

Example:

```text
./storage/IT23619944/dulsen/test.txt
```

---

## 3. Project Description

NetMessenger is a multi-client TCP chat and file-sharing application implemented in C using the standard BSD/POSIX socket API.

The system supports:

- Multiple simultaneous TCP clients
- User registration and duplicate username validation
- User listing
- Presence notifications
- Broadcast messaging
- Private messaging
- Chat rooms
- Room messaging
- File sharing
- Error handling
- Graceful and ungraceful disconnect handling
- Server-side logging
- Rate limiting / flood protection as an optional extension

The application was developed and tested on Linux using GCC.

---

## 4. System Architecture

NetMessenger follows a client-server architecture.

```text
+------------------+
|     Client 1     |
+--------+---------+
         |
         | TCP
         |
+--------v---------+
|                  |
|  NetMessenger    |
|      Server      |
|                  |
+--------+---------+
         |
         | TCP
         |
+--------v---------+
|     Client 2     |
+------------------+

        ...

+------------------+
|     Client 5     |
+------------------+
```

The server listens on TCP port **15944** and accepts multiple client connections.

A separate POSIX thread is created for each connected client. Shared client and room information is protected using pthread mutexes.

---

## 5. Concurrency Model

The server uses **POSIX threads (`pthread`)** to handle multiple clients concurrently.

For each incoming connection:

1. The server accepts the TCP connection.
2. A free client slot is assigned.
3. A separate pthread is created.
4. The thread handles registration and client commands.
5. Shared client and room state is protected using mutexes.

This design allows multiple clients to communicate with the server at the same time.

The implementation was tested using five simultaneous clients.

---

## 6. Communication Protocol

The implementation follows the specified line-based TCP protocol.

Every text command and response is terminated by `\n`.

The `SENDFILE` command is followed immediately by the specified number of raw file bytes.

### Supported Commands

```text
REGISTER <username>
LIST
BCAST <message>
PMSG <username> <message>
JOIN <room>
LEAVE <room>
ROOMS
RMSG <room> <message>
SENDFILE <target> <filename> <filesize>
QUIT
```

The interactive client accepts:

```text
SENDFILE <target> <filename>
```

and calculates the file size before sending the full protocol header to the server.

### REGISTER

Client command:

```text
REGISTER <username>
```

Successful response:

```text
OK REGISTERED <username> NID:6199
```

Duplicate usernames return:

```text
ERR 001 USERNAME_TAKEN NID:6199
```

### LIST

Client command:

```text
LIST
```

Example response:

```text
OK USERS dulsen,methsan,wijethunga,thisara,kawishka NID:6199
```

### BCAST

Client command:

```text
BCAST <message>
```

Sender receives:

```text
OK SENT NID:6199
```

Other connected clients receive:

```text
MSG BCAST <sender> <message>
```

### PMSG

Client command:

```text
PMSG <username> <message>
```

Successful response:

```text
OK SENT NID:6199
```

Target client receives:

```text
MSG PRIV <sender> <message>
```

Unknown users return:

```text
ERR 002 USER_NOT_FOUND NID:6199
```

### JOIN

Client command:

```text
JOIN <room>
```

Example:

```text
OK JOINED study NID:6199
```

A room is created automatically if it does not already exist.

### LEAVE

Client command:

```text
LEAVE <room>
```

Successful response:

```text
OK LEFT <room> NID:6199
```

If the room does not exist:

```text
ERR 003 ROOM_NOT_FOUND NID:6199
```

If the client is not a member of an existing room:

```text
ERR 003 NOT_IN_ROOM NID:6199
```

### ROOMS

Client command:

```text
ROOMS
```

Example response:

```text
OK ROOMS study,testroom NID:6199
```

### RMSG

Client command:

```text
RMSG <room> <message>
```

Successful response:

```text
OK SENT NID:6199
```

Room members receive:

```text
MSG ROOM <room> <sender> <message>
```

If the room does not exist:

```text
ERR 003 ROOM_NOT_FOUND NID:6199
```

If the client is not a room member:

```text
ERR 003 NOT_IN_ROOM NID:6199
```

### SENDFILE

Interactive client command:

```text
SENDFILE <target> <filename>
```

The client determines the file size and sends:

```text
SENDFILE <target> <filename> <filesize>
```

The raw file bytes follow immediately after the header, without an extra newline before or after the file data.

Successful response:

```text
OK FILE_RECEIVED <filename> NID:6199
```

Oversized files return:

```text
ERR 004 FILE_TOO_LARGE NID:6199
```

The target may be a username or a chat room.

### QUIT

Client command:

```text
QUIT
```

Response:

```text
OK BYE NID:6199
```

The server then closes the connection cleanly.

---

## 7. Error Handling

The server handles malformed and invalid requests without crashing.

Implemented error responses include:

```text
ERR 001 INVALID_COMMAND
ERR 001 USERNAME_TAKEN
ERR 001 NOT_REGISTERED
ERR 002 USER_NOT_FOUND
ERR 003 ROOM_NOT_FOUND
ERR 003 NOT_IN_ROOM
ERR 004 FILE_TOO_LARGE
ERR 005 SERVER_FULL
ERR 006 RATE_LIMITED
```

Server-generated `OK` and `ERR` response lines include the personalised NID tag:

```text
NID:6199
```

Forwarded message notifications such as `MSG BCAST`, `MSG PRIV`, `MSG ROOM`, and presence messages are sent in their specified notification format.

---

## 8. Presence Notifications

The server notifies other registered clients when users join or leave.

Join notification:

```text
MSG PRESENCE JOIN <username>
```

Leave notification:

```text
MSG PRESENCE LEAVE <username>
```

Presence behaviour was tested using multiple connected clients.

---

## 9. Disconnect Handling

The implementation supports both graceful and ungraceful disconnects.

### Graceful Disconnect

A client sends:

```text
QUIT
```

The server returns:

```text
OK BYE NID:6199
```

and closes the connection.

### Ungraceful Disconnect

When a client terminates unexpectedly:

- The user is removed from the active user list.
- The user is removed from all joined rooms.
- Empty rooms are cleaned up.
- Other registered clients receive a presence leave notification.
- The server continues operating without crashing.

---

## 10. File Sharing

Files can be sent to:

- A specific username
- A chat room

The server receives exactly the number of bytes specified by the file size.

The server stores a copy under:

```text
./storage/IT23619944/<sender_username>/<filename>
```

Example:

```text
./storage/IT23619944/dulsen/test.txt
```

The receiving client saves the transferred file as:

```text
received_<filename>
```

Example:

```text
received_test.txt
```

The implementation was tested using `test.txt` and `received_test.txt`, including file transfer to an individual user and to a room.

---

## 11. TCP Framing

The implementation uses helper functions for complete socket transmission and reception.

The server:

- Reads text commands line by line.
- Handles partial TCP receives.
- Receives exactly the required number of file bytes.
- Consumes file data correctly when an invalid target or oversized file is rejected.

The client uses complete-send/complete-receive helpers and line-based reading for server responses.

---

## 12. Server-Side Logging

Server events are stored in:

```text
netmsg_IT23619944.log
```

Timestamped events include:

- Server startup
- Client connections
- User registration
- Broadcast messages
- Private messages
- Room joins
- Room messages
- File transfers
- Client quits
- Client disconnections
- Rate-limit events

Example log entries:

```text
[2026-10-07 13:40:29] SERVER_START port=15944
[2026-10-07 13:40:59] REGISTER username=methsan
[2026-10-07 13:41:17] BCAST sender=methsan message=TEST message
[2026-10-07 13:41:55] PMSG sender=methsan target=dulsen message=Test private message
[2026-10-07 13:42:24] JOIN username=methsan room=testroom2
[2026-10-07 13:43:02] RMSG sender=methsan room=testroom2 message=Test room message
[2026-10-07 13:43:48] FILE sender=methsan target=dulsen filename=test.txt size=42
```

---

## 13. Optional Extension - Rate Limiting / Flood Protection

A basic per-client rate-limiting mechanism was implemented as an optional extension.

### Configuration

```text
Maximum messages: 5
Time window: 10 seconds
```

The rate limit applies to:

```text
BCAST
PMSG
RMSG
```

When the limit is exceeded, the server returns:

```text
ERR 006 RATE_LIMITED NID:6199
```

Example:

```text
Message 1 -> OK SENT
Message 2 -> OK SENT
Message 3 -> OK SENT
Message 4 -> OK SENT
Message 5 -> OK SENT
Message 6 -> ERR 006 RATE_LIMITED NID:6199
```

The optional extension was tested using six rapid BCAST messages from one client. The first five were accepted and the sixth was rejected with `RATE_LIMITED`.

---

## 14. Build Instructions

### Using the Personalised Makefile

From the project directory:

```bash
make -f Makefile_9944 clean
make -f Makefile_9944
```

This builds:

```text
server_9944
client_9944
```

### Manual Compilation

Server:

```bash
gcc -Wall -Wextra -pthread -o server_9944 server_9944.c
```

Client:

```bash
gcc -Wall -Wextra -o client_9944 client_9944.c
```

---

## 15. Running the Application

### Start the Server

```bash
./server_9944
```

Expected startup output:

```text
NetMessenger server started.
Listening on TCP port 15944...
```

### Start a Client

Open another terminal:

```bash
./client_9944
```

Enter a unique username when prompted.

Additional clients can be opened in separate terminals for simultaneous testing.

---

## 16. Personalisation Verification

The server listening port can be verified using:

```bash
ss -tlnp | grep 15944
```

The expected output contains port `15944`.

The personalised storage structure can be verified using:

```bash
find storage/IT23619944 -maxdepth 3 -type f -print
```

Example:

```text
storage/IT23619944/dulsen/test.txt
storage/IT23619944/methsan/test.txt
```

---

## 17. Testing Summary

| Test Case | Expected Result | Actual Result |
|---|---|---|
| User registration | User registered with NID tag | PASS |
| Duplicate username | `USERNAME_TAKEN` error | PASS |
| LIST command | Connected users displayed | PASS |
| Broadcast messaging | Message delivered to other clients | PASS |
| Private messaging | Message delivered only to target | PASS |
| Unknown user | `USER_NOT_FOUND` error | PASS |
| JOIN / room creation | Room created and client joined | PASS |
| ROOMS command | Available rooms listed | PASS |
| Room messaging | Message delivered to room members | PASS |
| LEAVE command | Client leaves room successfully | PASS |
| Unknown room | `ROOM_NOT_FOUND` error | PASS |
| Non-member room message | `NOT_IN_ROOM` error | PASS |
| File transfer to user | Complete file delivered | PASS |
| File transfer to room | Complete file delivered to room members | PASS |
| Invalid file target | Appropriate error returned and connection remains usable | PASS |
| File size limit | `FILE_TOO_LARGE` error | PASS |
| Five simultaneous clients | Server handles concurrent clients | PASS |
| Presence JOIN | Other clients receive join notification | PASS |
| Presence LEAVE | Other clients receive leave notification | PASS |
| Graceful disconnect | User cleaned up after `QUIT` | PASS |
| Ungraceful disconnect | User and room state cleaned up | PASS |
| Invalid command | `INVALID_COMMAND` returned without crash | PASS |
| Server-side logging | Events recorded with timestamps | PASS |
| Personalised port | Server listens on `15944` | PASS |
| Personalised storage | File stored under personalised path | PASS |
| Rate limiting | 6th rapid message rejected | PASS |

---

## 18. Project Files

The main submission files are:

```text
NetMessenger/
├── server_9944.c
├── client_9944.c
├── Makefile_9944
├── README.md
├── Design_Diary.md
├── Prompt_Log.md
├── Reflection.md
└── netmsg_IT23619944.log
```

Test files used during development include:

```text
test.txt
received_test.txt
storage/IT23619944/
```

Generated executable files are:

```text
server_9944
client_9944
```

---

## 19. Technologies Used

- C
- GCC
- TCP/IP
- BSD/POSIX sockets
- pthread
- Linux
- Git
- GitHub

---

## 20. Git Development

The project was developed incrementally using Git with descriptive commits.

The current Git history contains the following 13 commits:

1. Initialize NetMessenger Project
2. Implement multi-client registration and duplicate validation
3. Implement interactive client and LIST command
4. Implement broadcast messaging
5. Implement private messaging and error handling
6. Implement chat room messaging
7. Implement file sharing
8. Improve protocol robustness and project documentation
9. Add rate limiting and flood protection
10. Update README with Optional Extension
11. Add design diary and development decisions
12. Add AI prompt log and interaction record
13. Add structured learning reflection

GitHub repository:

```text
https://github.com/dulsen-methsan/NetMessenger
```

---

## 21. Development and Validation

The application was compiled and tested on Linux using GCC.

Testing covered:

- Functional protocol commands
- Multiple simultaneous clients
- Duplicate username handling
- Broadcast and private messaging
- Chat rooms
- File transfer
- File-size validation
- Invalid target handling
- Presence notifications
- Graceful and ungraceful disconnects
- User and room cleanup
- Server-side logging
- Personalised port and storage verification
- Rate limiting / flood protection

The final implementation was validated through runtime testing before preparing the submission documentation.
