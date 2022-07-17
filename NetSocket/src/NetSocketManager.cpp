#pragma once

#include "NetSocketManager.h"

CNetSocketManager::CNetSocketManager()
{
    
}

CNetSocketManager::~CNetSocketManager(void)
{
    CloseAllSockets();
}

SOCKET CNetSocketManager::InitSocketUDP(const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag)
{
    //Set 

    return 0;
}

bool CNetSocketManager::InitSocketUDP(SOCKET socket, const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag)
{
    return false;
}

bool CNetSocketManager::CloseSocket(SOCKET socket)
{
    if (closesocket(socket) != SOCKET_ERROR)
        return (WSACleanup() == 0);
    else
        return false;
}

bool CNetSocketManager::CloseAllSockets()
{
    //Close all UDP sockets
    for (auto& i : lvUDPSocketMap)
    {
        if (CloseSocket(i.first))
            lvUDPSocketMap.erase(i.first);
    }
    //Close all TCP sockets
    for (auto& i : lvTCPSocketMap)
    {
        if (CloseSocket(i.first))
            lvTCPSocketMap.erase(i.first);
    }

    return lvUDPSocketMap.empty() && lvTCPSocketMap.empty();
}

uint16_t CNetSocketManager::GetRxBufferSize(SOCKET socket)
{
    //Check if socket is UDP
    if (lvUDPSocketMap.find(socket) != lvUDPSocketMap.end())
        return lvUDPSocketMap[socket].RxBufferSize;
    else
    {
        //Check if socket is TCP
        if (lvTCPSocketMap.find(socket) != lvTCPSocketMap.end())
            return lvTCPSocketMap[socket].RxBufferSize;
        else
            return 0;
    }
}

void CNetSocketManager::SetRxBufferSize(SOCKET socket, const uint16_t& newSize)
{
    //Check if socket is UDP
    if (lvUDPSocketMap.find(socket) != lvUDPSocketMap.end())
        ResizeBuffer(lvUDPSocketMap[socket].RxBuffer, lvUDPSocketMap[socket].RxBufferSize, newSize);
    else
    {
        //Check if socket is TCP
        if (lvTCPSocketMap.find(socket) != lvTCPSocketMap.end())
            ResizeBuffer(lvTCPSocketMap[socket].RxBuffer, lvTCPSocketMap[socket].RxBufferSize, newSize);
    }
}

uint16_t CNetSocketManager::GetTxBufferSize(SOCKET socket)
{
    //Check if socket is UDP
    if (lvUDPSocketMap.find(socket) != lvUDPSocketMap.end())
        return lvUDPSocketMap[socket].TxBufferSize;
    else
    {
        //Check if socket is TCP
        if (lvTCPSocketMap.find(socket) != lvTCPSocketMap.end())
            return lvTCPSocketMap[socket].TxBufferSize;
        else
            return 0;
    }
}

void CNetSocketManager::SetTxBufferSize(SOCKET socket, const uint16_t& newSize)
{
    //Check if socket is UDP
    if (lvUDPSocketMap.find(socket) != lvUDPSocketMap.end())
        ResizeBuffer(lvUDPSocketMap[socket].TxBuffer, lvUDPSocketMap[socket].TxBufferSize, newSize);
    else
    {
        //Check if socket is TCP
        if (lvTCPSocketMap.find(socket) != lvTCPSocketMap.end())
            ResizeBuffer(lvTCPSocketMap[socket].TxBuffer, lvTCPSocketMap[socket].TxBufferSize, newSize);
    }
}

uint16_t& CNetSocketManager::GetMaxTCPConnections(SOCKET socket)
{
    return lvTCPSocketMap[socket].PeerLimit;
}

void CNetSocketManager::SetMaxTCPConnections(SOCKET socket, const uint16_t& maxNum)
{
    if (lvTCPSocketMap.find(socket) != lvTCPSocketMap.end())
    {
        while (lvTCPSocketMap[socket].Peers.size() > maxNum)
        {
            //Cleanup and erase the oldest connected peer.
            auto iter = lvTCPSocketMap[socket].Peers.begin();

            CloseSocket(iter->first);
            lvTCPSocketMap[socket].Peers.erase(iter);
        }
        lvTCPSocketMap[socket].PeerLimit = maxNum;
    }
}

inline bool CNetSocketManager::ResizeBuffer(char* buffer, uint16_t& size, const uint16_t& newSize)
{
    if (buffer != nullptr)
    {
        char* newBuffer = new char[newSize];
        memset(newBuffer, '\0', sizeof(char) * newSize);

        memcpy(newBuffer, buffer, sizeof(char) * min(newSize, size));
        delete[] buffer;
        buffer  = newBuffer;
        size    = newSize;

        return true;
    }
    else
        return false;
}
