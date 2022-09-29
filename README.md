# NetSocket
A useful header-only C++ library that aims to be a simpler, more robust way to integrate network sockets into other projects.

## Requirements
- C++11 and above.

## Modes of Operation
- TCP Server
- TCP Client
- UDP Server
- UDP (P2P)

## Examples
- HTTPRequest: A simple console application which initializes a TCP client socket, and performs a "GET" request from a URL, or an IP address.
- TCPServerGUI: A MFC application that operates as a server for handling traffic between instant messaging clients. This example is paired up with the TCPClientGUI project.
- TCPClientGUI (WIP): A MFC application that operates as a basic instant messaging client, and uses the TCPServerGUI as a gateway to other connected clients. This application is still in development, and will be completed soon.

## Future Developments & Improvements
- Completion of the TCPClientGUI example.
- Unix compatibility.
- IPv6 compatibility.
