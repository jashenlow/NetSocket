#pragma once

#include <fcntl.h>
#include "../../NetSocket.h"
#include "../../NetUtil.h"
#include <sstream>

#include <thread>

int main(int argc, char** argv)
{
    //_setmode(_fileno(stdout), _O_U16TEXT); //To set console in unicode.

    NetSocket testSocket;
    WinsockError errorCode;

    errorCode = testSocket.Init("192.168.10.5", 80, SocketType::TCP_CLIENT, false); //Change IP to console argument.

    if (errorCode == WinsockError::OK)
    {
        const NetSocket::LocalSocketInfo& localInfo = testSocket.GetThisSocketInfo();

        printf("\tLocal HostName: %s\n", localInfo.HostName);
        printf("\tLocal IP: %s\n", localInfo.IPAddress.c_str());
        printf("\tLocal Port: %d\n", localInfo.Port);

        std::string strURL = "httpbin.org";

        bool isConnected = false;
        int errConnect = testSocket.ConnectToServerTCP(strURL.c_str(), 80);

        if (errConnect == ConnectingErrCode)
        {
            int errWaitConnect = testSocket.CheckForConnectNonBlockTCP(2.0);

            switch (errWaitConnect)
            {
                case 1:
                    printf("Connected to %s\n", strURL.c_str());
                    isConnected = true;
                    break;
                case 0:
                    printf("Connection to %s timeout expired.\n", strURL.c_str());
                    break;
                case SOCKET_ERROR:
                    printf("Failed to connect to %s\n", strURL.c_str());
                    break;
            }
        }
        else if (errConnect == 0)
        {
            isConnected = true;
            printf("Connected to %s\n", strURL.c_str());
        }
        else
            printf("Connection to %s failed.\n", strURL.c_str());

        if (isConnected)
        {
            const NetSocket::SocketInfo& serverInfo = testSocket.GetRemoteSocketInfo().front();

            printf("\tHostName: %s\n", serverInfo.HostName);
            printf("\tServer IP: %s\n", serverInfo.IPAddress.c_str());
            printf("\tServer Port: %d\n", serverInfo.Port);
            printf("\n");

            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::string request;
            request += "GET / HTTP/1.1\r\n";
            request += "Host: " + strURL + "\r\n";
            request += "Connection: close\r\n\r\n";

            if (testSocket.SendToServerTCP(request.c_str(), request.length() + 1))
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
                    recvBytes = testSocket.ReceiveFromServerTCP(ss);
                    
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
                    recvBytes = testSocket.ReceiveFromServerTCP(contentStr);

                    if (contentStr.length() != contentLength)
                    {
                        timeoutTimer += (10.0 / 1000.0);

                        if (timeoutTimer >= 5.0)
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

    testSocket.CloseThisSocket();
    system("pause");
}

