# Cchat
Practice network programming

The plan:
 - [x] A server application, that hosts exactly one chat room and accepts connections
 - [x] A client application, that connects to the chat
 - [x] TUI for a client application, fully keyboard driven
 - [x] No authentication - anyone can enter anonymously or with a username as long as it is not acquired at the moment


# Instruction

1. Clone repo and compile project with `make`.
2. If you want to host chatroom - make sure you have public IP adress available on the intenet and your port is exposed by the firewall.
3. Default port is 12321, you can use you port by passing `D` CLI arguement (`./cchat D 32123`).
4. Run application:
    - For hosting you can just run `./cchat`.
    - For connecting you have to run it with `H` CLI arguement (`./cchat H 127.0.0.1`)

# Further improvements

I am not planning to improve usability of this application any further. This is just an excersise application and if for some reason you reading it with intention of actually using it, I strongly recommend you to find an alternative that works better.

# Conclusion

That being said, I enjoyed creating it. It helped me learn a little bit of:
- Multithreaded programming in C with `pthread.h`.
- Network programming in C with `socket.h`.
- TUI programming.
