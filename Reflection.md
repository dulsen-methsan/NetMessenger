# Reflection

## IE3010 - Network Programming
### NetMessenger

**Registration Number:** IT23619944  
**Node ID (NID):** 6199

During the development of the NetMessenger assignment, I used ChatGPT as a support tool at selected stages of the project. I mainly used it for understanding the assignment requirements, reviewing socket-programming implementation ideas, debugging protocol-related issues, planning tests, and preparing documentation. I did not treat the generated suggestions as automatically correct. The implementation was compiled and tested in my Linux virtual machine before accepting changes.

One useful aspect of AI assistance was helping me understand how the different parts of a TCP client-server application should work together. It was also useful when reviewing the protocol and identifying possible problems in error handling and file transfer. However, the AI suggestions were not always correct on the first attempt. For example, during room testing, the implementation initially returned NOT_IN_ROOM when a requested room did not exist. After comparing the actual output with the assignment protocol, I changed the logic so that a missing room returns ROOM_NOT_FOUND, while a connected user who is not a member receives NOT_IN_ROOM.

File sharing also showed me the importance of TCP framing. The SENDFILE command requires the server to receive exactly the specified number of raw bytes. During testing, I found that an invalid file target could affect the handling of the following data. The implementation was modified so that the expected file bytes are consumed before the error response is returned, allowing the same connection to continue operating correctly. I verified this by sending another LIST command after the file error.

The assignment improved my practical understanding of concurrent network programming. I learned how pthreads can be used to handle multiple clients at the same time and why shared client and room state needs synchronization. I also gained a better understanding of the difference between graceful and ungraceful disconnections and how a server should clean up user and room state.

The testing process was an important part of my learning. I tested five simultaneous clients, presence notifications, messaging, chat rooms, file sharing, invalid commands, disconnects, logging, and personalised configuration values. I also implemented and tested a rate-limiting mechanism as an optional extension.

Overall, the project helped me understand network programming beyond simply writing socket code. In particular, I learned that correct protocol framing, state management, concurrency, error handling, and repeated runtime testing are all essential for building a reliable client-server application.
