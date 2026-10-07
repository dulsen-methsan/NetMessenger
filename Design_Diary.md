# Design Diary

## IE3010 - Network Programming
### NetMessenger: A Multi-Client Chat and File-Sharing Platform

**Registration Number:** IT23619944  
**Node ID (NID):** 6199  
**Server Port:** 15944

### Development Journey

The NetMessenger project was implemented as a TCP client-server application in C using the standard BSD/POSIX socket API. The main design decision was to use a separate POSIX thread for each connected client. This approach was selected because it provides a straightforward way to support multiple simultaneous clients while keeping the client-handling logic separated. Shared client and room information is protected using mutexes to reduce conflicts between threads.

The implementation was developed incrementally. The initial stage focused on establishing the TCP server and client structure, followed by user registration and duplicate username validation. The LIST command was then added to maintain and display the currently connected users. Broadcast and private messaging were implemented next, followed by chat-room functionality including JOIN, LEAVE, ROOMS and RMSG.

File sharing introduced additional implementation challenges because the protocol requires the server to receive exactly the specified number of raw file bytes after the SENDFILE command. Helper functions were introduced to support complete sending and receiving of data. The server also stores a copy of transferred files under the personalised storage path.

During testing, several protocol and error-handling issues were identified and corrected. In particular, LEAVE and RMSG initially returned NOT_IN_ROOM when a room did not exist. This was changed so that a non-existent room returns ROOM_NOT_FOUND, while a user who is not a member of an existing room receives NOT_IN_ROOM. SENDFILE handling was also improved so that invalid or oversized file transfers do not leave the TCP connection in an inconsistent state.

Disconnect handling was tested using both the QUIT command and unexpected client termination. The server successfully removed disconnected users from the active user list and room membership, and empty rooms were cleaned up.

As an optional extension, a basic rate-limiting mechanism was implemented. Each client is allowed to send up to five chat messages within a ten-second window. Further messages are rejected with the RATE_LIMITED response. This was tested using rapid broadcast messages.

The final stages focused on improving the project README, creating a personalised Makefile, verifying the personalised port, storage path and log file, and testing the system using multiple simultaneous clients.

### Key Lessons and Decisions

The main lesson from the project was the importance of TCP message framing and careful state management when handling multiple clients. Testing also showed that protocol errors should be handled without disrupting the remaining client connection. The incremental testing process helped identify these issues before finalising the implementation.
