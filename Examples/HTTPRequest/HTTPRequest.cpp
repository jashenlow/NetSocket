#pragma once

#include <fcntl.h>
#include "../../NetSocket.h"
#include "../../NetUtil.h"
#include <sstream>

#include <thread>

constexpr const char* URL = "httpbin.org";

int main(int argc, char** argv)
{
    //_setmode(_fileno(stdout), _O_U16TEXT); //To set console in unicode.
    std::string ipAddress;

    if (argv[1] != nullptr)
        ipAddress = argv[1];

    //Check for empty string
    if (!ipAddress.empty())
    {
        NetSocket thisSocket;
        WinsockError errorCode;

        errorCode = thisSocket.Init(ipAddress.c_str(), 80, SocketType::TCP_CLIENT, true);
        
        if (errorCode == WinsockError::OK)
        {
            const NetSocket::LocalSocketInfo& localInfo = thisSocket.GetThisSocketInfo();
            
            printf("\tLocal HostName: %s\n", localInfo.HostName);
            printf("\tLocal IP: %s\n", localInfo.IPAddress);
            printf("\tLocal Port: %d\n", localInfo.Port);

            bool isConnected = false;
            int errConnect = thisSocket.ConnectToServerTCP(URL, 80);

            if (errConnect == NonBlockIncomplete)
            {
                int errWaitConnect = thisSocket.CheckForConnectNonBlockTCP(2.0);

                switch (errWaitConnect)
                {
                    case 1:
                        printf("Connected to %s\n", URL);
                        isConnected = true;
                        break;
                    case 0:
                        printf("Connection to %s timeout expired.\n", URL);
                        break;
                    case SOCKET_ERROR:
                        printf("Failed to connect to %s\n", URL);
                        break;
                }
            }
            else if (errConnect == 0)
            {
                isConnected = true;
                printf("Connected to %s\n", URL);
            }
            else
                printf("Connection to %s failed.\n", URL);

            if (isConnected)
            {
                const NetSocket::SocketInfo& serverInfo = thisSocket.GetRemoteSocketInfo().front();

                printf("\tHostName: %s\n", serverInfo.HostName);
                printf("\tServer IP: %s\n", serverInfo.IPAddress);
                printf("\tServer Port: %d\n", serverInfo.Port);
                printf("\n");

                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                std::string request;
                request += "GET / HTTP/1.1\r\n";
                request += "Host: ";
                request += URL;
                request += "\r\n";
                request += "Connection: close\r\n\r\n";

                if (thisSocket.SendToServerTCP(request.c_str(), request.length() + 1))
                {
                    constexpr const char* RESP_HEADER_END = "\r\n";
                    int recvBytes = 0;
                    int totalRecvBytes = 0;
                    size_t contentLength = 0;
                    std::stringstream ss;
                    double timeoutTimer = 0.0;
                    bool fullHeaderReceived = false;

                    //Read response header
                    while (recvBytes <= 0)
                    {
                        recvBytes = thisSocket.ReceiveFromServerTCP(ss);

                        if (recvBytes > 0)
                        {
                            std::string recvStr = ss.str();
                            printf("%s\n", recvStr.c_str());

                            fullHeaderReceived = (recvStr.substr(recvStr.length() - 2, 2) == RESP_HEADER_END);

                            if (fullHeaderReceived)
                            {
                                bool isResponse = false;
                                std::string lineStr;

                                while (std::getline(ss, lineStr))
                                {
                                    if (lineStr.find("HTTP") != std::string::npos)
                                        isResponse = true;

                                    if (isResponse)
                                    {
                                        //Look for "Content-Length"
                                        if (lineStr.find("Content-Length") != std::string::npos)
                                        {
                                            size_t delimPos = lineStr.find(':');
                                            if (delimPos != std::string::npos)
                                            {
                                                size_t numberPos = delimPos + 2;
                                                contentLength = strtol(lineStr.substr(numberPos, lineStr.length() - numberPos).c_str(), nullptr, 10);
                                                break;
                                            }
                                            else
                                            {
                                                printf("Unable to find content length from header. Exiting...\n");
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        printf("Unrecognized response received. Exiting...\n");
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            timeoutTimer += (10.0 / 1000.0);

                            if (timeoutTimer >= 5.0)
                            {
                                printf("Failed to get response. Exiting...\n");
                                break;
                            }

                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                    }

                    //Read page content
                    std::string contentStr;
                    timeoutTimer = 0.0;

                    while (contentStr.length() != contentLength)
                    {
                        recvBytes = thisSocket.ReceiveFromServerTCP(contentStr);

                        if (contentStr.length() != contentLength)
                        {
                            timeoutTimer += (10.0 / 1000.0);

                            if (timeoutTimer >= 10.0)
                            {
                                printf("Failed to get content. Exiting...\n");
                                break;
                            }

                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                        else
                            break;
                    }

                    printf("%s\n", contentStr.c_str());
                }
            }
        }
        else
            printf("Error initializing socket.\n");

        thisSocket.CloseThisSocket();
        system("pause");
    }
    else
        printf("Error: Please input your machine's IP address as an argument.\n");
}

