# NetSocket
A useful header-only C++ library that aims to be a simpler, more robust way to implement network sockets.

## Requirements
- C++11 and above.

## Modes of Operation
- TCP Server
- TCP Client
- UDP Server
- UDP (P2P)

## Examples
- HTTPRequest: A simple console application which initializes a TCP client socket, and performs a "GET" request from a domain, or an IP address.
- TCPServerGUI: A MFC application that operates as a server for handling traffic between instant messaging clients. This example is paired up with the TCPClientGUI project.
- TCPClientGUI: A MFC application that operates as a basic instant messaging client, and uses the TCPServerGUI as a gateway to other connected clients.

## Future Developments & Improvements
- More testing for the TCPServerGUI and TCPClientGUI examples.
- Unix compatibility.
- IPv6 compatibility.
