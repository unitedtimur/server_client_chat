#pragma once

#include <iostream>

namespace Configuration
{
	static std::string COPYRIGHT				= "UnitedTimur (c)";
	static std::string ERROR_CREATE_SOCKET		= "error -> can not create a socket!";
	static std::string ERROR_CREATE_WSADATA		= "error -> can not initialize winsock!";
	static std::string ERROR_GETADDR_INFO		= "error -> getaddrinfo failed!";

	static std::string EXIT_SYMBOL				= ":wq";

	static const int BUFFER_SIZE				= 1024;

	namespace Server
	{
		static std::string START				= "has been started...";
		static std::string LISTEN				= "is listening on port";
		static std::string RECV					= "is receiving data...";
		static std::string NEW_CONNECTION		= "new client connection!";
		static std::string CLIENT_DISCONNECT	= "client has been disconnected!";
		static std::string ERROR_WSADATA		= ERROR_CREATE_WSADATA;
		static std::string ERROR_SOCKET			= ERROR_CREATE_SOCKET;
		static std::string ERROR_SETSOCKOPT		= "error -> setsockopt failed!";
		static std::string ERROR_GETADDRINFO	= ERROR_GETADDR_INFO;
		static std::string ERROR_BIND			= "error -> unable to establish bind connection!";
		static std::string ERROR_LISTEN			= "error -> can not listen the socket!";
		static std::string ERROR_ACCEPT_FAILED	= "error -> accept failed!";
		static std::string ERROR_SEND			= "error -> send error!";
		static std::string ERROR_RECV			= "error -> recv error!";
		static std::string ERROR_SELECT			= "error -> select error!";
	}

	namespace Client
	{
		static std::string START				= "has been connected...";
		static std::string LISTEN				= "is listening server...";
		static std::string INVALID_HOST			= "error -> invalid address or not supported address!";
		static std::string ERROR_WSADATA		= ERROR_CREATE_WSADATA;
		static std::string ERROR_SOCKET			= ERROR_CREATE_SOCKET;
		static std::string ERROR_GETADDRINFO	= ERROR_GETADDR_INFO;
		static std::string ERROR_CONNECT		= "error -> connection failed!";
	}
}