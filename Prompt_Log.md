# Prompt Log

## IE3010 - Network Programming
### NetMessenger: A Multi-Client Chat and File-Sharing Platform

**Registration Number:** IT23619944  
**Node ID (NID):** 6199  
**Server Port:** 15944

This prompt log records the substantive use of AI assistance during the development of Part 1 of the NetMessenger assignment.

AI was used as a development support tool for selected implementation, debugging, protocol review, testing and documentation tasks. The suggestions were checked against the assignment specification and validated through compilation and runtime testing in the Linux virtual machine.

---

## 1. Understanding the Assignment Specification

**Tool:** ChatGPT

**Purpose / Prompt:**  
Reviewed the IE3010 Network Programming assignment specification to identify the mandatory functionality, communication protocol, personalisation requirements, testing requirements and submission deliverables.

**How the output was used:**  
The assignment requirements were converted into a practical development and testing checklist. Particular attention was given to the required commands, personalised port and NID, file storage path, logging, screenshots, Git commits, design diary, prompt log and reflection.

**My evaluation / changes:**  
The assignment PDF was treated as the primary source. AI suggestions were checked against the actual specification before being used.

---

## 2. Initial TCP Client-Server Implementation

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested guidance and implementation assistance for building the basic NetMessenger TCP server and client in C using the standard BSD/POSIX socket API.

**How the output was used:**  
The AI-assisted code structure was used as a starting point for the socket-based client-server implementation. The source files were placed in the Linux VM and compiled with GCC.

**My evaluation / changes:**  
The code was not accepted without testing. Compilation results and runtime behaviour were checked in the VM, and subsequent changes were made when errors or protocol issues were identified.

---

## 3. Personalisation of the Application

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested help calculating and applying the personalised values required by the assignment using registration number IT23619944.

**How the output was used:**  
The values were identified as:

- Server/client port: 15944
- NID: 6199
- Server source: server_9944.c
- Client source: client_9944.c
- Makefile: Makefile_9944
- Log file: netmsg_IT23619944.log
- Storage root: ./storage/IT23619944/

**My evaluation / changes:**  
The calculations were checked against the formula in the assignment specification and verified through actual server output, the listening-port check and the storage directory.

---

## 4. Registration and User Management

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested assistance with implementing and testing user registration, unique username validation and the LIST command.

**How the output was used:**  
The suggestions were used to support registration and user-list handling.

**My evaluation / changes:**  
The implementation was tested using multiple clients. Duplicate usernames were deliberately tested and the result was verified as:

ERR 001 USERNAME_TAKEN NID:6199

The LIST command was also tested with multiple simultaneous users.

---

## 5. Broadcast and Private Messaging

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested implementation and testing guidance for BCAST and PMSG functionality and the required response formats.

**How the output was used:**  
The message-routing logic and response handling were reviewed with AI assistance and then tested using separate client sessions.

**My evaluation / changes:**  
The sender responses and the messages received by other clients were compared with the protocol specification. An invalid private-message target was also tested to verify USER_NOT_FOUND handling.

---

## 6. Chat Room Functionality

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested assistance with the JOIN, LEAVE, ROOMS and RMSG room-management functions.

**How the output was used:**  
AI assistance was used to review room membership logic and to identify places where the implementation needed clearer error handling.

**My evaluation / changes:**  
The room commands were tested using multiple clients. The original implementation treated a missing room and a non-member in the same way. During testing, this was identified as inconsistent with the protocol.

The implementation was changed so that:

- A non-existent room returns ROOM_NOT_FOUND.
- An existing room with a non-member returns NOT_IN_ROOM.

Both cases were re-tested after the change.

---

## 7. File Sharing and TCP Framing

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested assistance with SENDFILE implementation, exact byte reception, server-side file storage and error handling.

**How the output was used:**  
AI guidance was used to review the handling of the file header and the exact number of file bytes that must be received. The server stores files using the personalised storage path.

**My evaluation / changes:**  
File transfer was tested using test.txt and received_test.txt. The transferred file was also checked after reception.

Additional testing identified a framing issue when an invalid file target was used. The server was then changed to consume the expected file bytes before returning the error so that the same TCP connection remained usable.

The corrected implementation was tested again by sending an invalid target and then successfully issuing LIST on the same connection.

---

## 8. File Size Error Handling

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested a way to test and verify the FILE_TOO_LARGE condition and the behaviour of the connection after an oversized transfer.

**How the output was used:**  
A test was performed using a file size above the configured maximum.

**My evaluation / changes:**  
The server returned:

ERR 004 FILE_TOO_LARGE NID:6199

A LIST command was then issued afterwards to confirm that the connection remained usable. The test passed.

---

## 9. Disconnect and Presence Testing

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested testing guidance for presence notifications and graceful/ungraceful client disconnections.

**How the output was used:**  
The testing process was used to verify JOIN and LEAVE presence messages and to check that disconnected users were removed from the active user list.

**My evaluation / changes:**  
A client was disconnected normally using QUIT and another client was terminated unexpectedly. The server continued running, sent the appropriate leave notification, removed the user from the user list and cleaned up room membership.

The tests were repeated after implementation changes to confirm that the behaviour remained correct.

---

## 10. Room Cleanup After Unexpected Disconnect

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested assistance checking whether disconnected clients were correctly removed from room membership.

**How the output was used:**  
A room was created and a client joined it. The client was then disconnected unexpectedly and the ROOMS command was used from another client.

**My evaluation / changes:**  
The test showed that the disconnected user was removed from the room and the empty room was cleaned up. The result was verified using actual terminal output.

---

## 11. Error Handling and Invalid Commands

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested examples and testing guidance for malformed commands and invalid requests.

**How the output was used:**  
The server was tested with an invalid command:

HELLO

The server returned:

ERR 001 INVALID_COMMAND NID:6199

**My evaluation / changes:**  
The behaviour was verified in the running application. The server remained operational after the invalid command.

---

## 12. Build System and Makefile

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested assistance creating a personalised Makefile for server_9944.c and client_9944.c.

**How the output was used:**  
A personalised Makefile was created with targets for building, cleaning and rebuilding the server and client.

**My evaluation / changes:**  
The development environment did not initially have GNU Make installed. The environment was identified as CentOS Stream 10, after which GNU Make was installed using the available package manager.

The Makefile was then tested with:

make -f Makefile_9944 clean
make -f Makefile_9944

The server and client compiled successfully.

---

## 13. README Documentation

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested assistance preparing the README with the personalised project details, build instructions, protocol summary, testing information, logging details and optional extension information.

**How the output was used:**  
The README was updated to describe the project configuration, commands, testing and build process.

**My evaluation / changes:**  
The README was checked against the actual project values and implementation rather than being treated as independent documentation.

---

## 14. Rate Limiting / Flood Protection

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested guidance for implementing an optional rate-limiting / flood-protection extension.

**How the output was used:**  
A basic per-client rate limit was implemented for BCAST, PMSG and RMSG.

The configured behaviour is:

- Maximum 5 messages
- 10-second time window

When the limit is exceeded, the server returns:

ERR 006 RATE_LIMITED NID:6199

**My evaluation / changes:**  
The feature was compiled and tested in the Linux VM. Six rapid BCAST messages were sent from one client.

The first five messages returned OK SENT and the sixth returned RATE_LIMITED. The result was used as evidence that the optional extension was working.

---

## 15. Protocol Error Corrections

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested a review of protocol mismatches discovered during runtime testing.

**How the output was used:**  
The assistant helped identify differences between actual responses and the required protocol.

**My evaluation / changes:**  
The following corrections were made after testing:

- LEAVE on a non-existent room -> ROOM_NOT_FOUND
- RMSG on a non-existent room -> ROOM_NOT_FOUND
- Non-member room operation -> NOT_IN_ROOM
- Invalid SENDFILE target -> error without breaking the connection
- Oversized SENDFILE -> FILE_TOO_LARGE while preserving the connection state

Each change was tested after implementation.

---

## 16. Testing and Evidence Collection

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested guidance on which terminal outputs and implementation results should be captured as evidence for the Implementation Report.

**How the output was used:**  
The testing process was organised into evidence for:

- Five simultaneous clients
- Presence JOIN and LEAVE
- User and room cleanup
- Invalid commands
- Duplicate usernames
- File transfer
- File-size errors
- Invalid file targets
- Server logging
- Personalised storage
- Server listening port
- Rate limiting

**My evaluation / changes:**  
Only actual runtime outputs from the Linux VM were used as evidence. Screenshots were captured from the running project.

---

## 17. Git and Development Process

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested guidance on organising meaningful Git commits and checking the repository before final submission.

**How the output was used:**  
The project was reviewed incrementally and meaningful changes were committed during development.

The main development history includes commits for:

1. Project initialisation
2. Multi-client registration and duplicate validation
3. Interactive client and LIST command
4. Broadcast messaging
5. Private messaging and error handling
6. Chat room messaging
7. File sharing
8. Protocol robustness and project documentation
9. Rate limiting and flood protection
10. README update with optional extension
11. Design diary and development decisions
12. AI prompt log and interaction record
13. structured learning reflection

**My evaluation / changes:**  
The commit history was reviewed using Git commands and pushed to the GitHub repository. The GitHub remote and push result were also verified.

---

## 18. Design Diary and Submission Documentation

**Tool:** ChatGPT

**Purpose / Prompt:**  
Requested assistance with structuring the design diary and other assignment documentation based on the actual development process.

**How the output was used:**  
The documentation structure was prepared around the real implementation decisions, obstacles and testing activities.

**My evaluation / changes:**  
The content was checked against the work completed in the Linux VM and the Git development history.

---

## 19. Overall Evaluation of AI Assistance

AI was used as a support tool during selected parts of the development process. It was particularly useful for explaining socket-programming concepts, reviewing protocol handling, suggesting debugging approaches, and assisting with documentation.

However, AI suggestions were not treated as automatically correct. The implementation was compiled, executed and tested in the Linux environment. Several problems were discovered during practical testing and then corrected, including room error responses and SENDFILE connection framing.

The testing process was important for understanding how TCP framing, concurrent client handling, shared server state, file transfer and disconnect cleanup behave in a real socket application.

The final implementation decisions were based on the assignment specification and the observed behaviour of the running program.
