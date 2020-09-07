#include "server.h"
#include "../src/configuration.h"
#include "WS2tcpip.h"

#include <thread>
#include <mutex>
#include <sstream>

std::mutex mutex;

Server::Server(const std::string& host, const std::string& port) : 
	host(host),
	port(port),
	serverAddress(nullptr)
{
}

Server::~Server()
{
	freeaddrinfo(serverAddress);
	WSACleanup();
}

void Server::start()
{
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);

	if (WSAStartup(version, &wsaData) != 0)
	{
		this->notify(Configuration::Server::ERROR_WSADATA);
		return;
	}

	addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &serverAddress) != 0)
	{
		this->notify(Configuration::Server::ERROR_GETADDRINFO);
		return;
	}

	if ((this->serverSocket = socket(serverAddress->ai_family, serverAddress->ai_socktype, serverAddress->ai_protocol)) == INVALID_SOCKET)
	{
		this->notify(Configuration::Server::ERROR_SOCKET);
		return;
	}

	if (bind(this->serverSocket, serverAddress->ai_addr, int(serverAddress->ai_addrlen)) == SOCKET_ERROR)
	{
		this->notify(Configuration::Server::ERROR_BIND);
		closesocket(this->serverSocket);
		return;
	}

	if (listen(this->serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		this->notify(Configuration::Server::ERROR_LISTEN);
		closesocket(this->serverSocket);
		return;
	}

	this->notify(Configuration::Server::START);

	std::thread threadListen(&Server::server_listen, this);
	std::thread threadRecv(&Server::recv_info, this);

	threadRecv.detach();
	threadListen.join();
}

void Server::server_listen()
{
	this->notify(Configuration::Server::LISTEN + ' ' + this->port);

	std::string response = Configuration::COPYRIGHT;

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(75));

		SOCKET clientSocket;

		if ((clientSocket = accept(this->serverSocket, 0, 0)) == INVALID_SOCKET)
		{
			this->notify(Configuration::Server::ERROR_ACCEPT_FAILED);
			closesocket(clientSocket);
			continue;
		}

		if (!this->server_send(clientSocket, response))
		{
			this->notify(Configuration::Server::ERROR_SEND);
			closesocket(clientSocket);
			continue;
		}

		this->notify(Configuration::Server::NEW_CONNECTION);
		this->push_back(clientSocket);
	}
}

void Server::recv_info()
{
	this->notify(Configuration::Server::RECV);

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(75));

		const auto sockets = this->clients;

		for (const auto& socket : sockets)
		{
			std::string message; 

			if (!this->server_recv(socket, message))
			{
				this->notify(Configuration::Server::CLIENT_DISCONNECT);
				this->erase(socket);
				continue;
			}
			
			if (this->is_client_connection_close(message))
			{
				this->notify(Configuration::Server::CLIENT_DISCONNECT);
				this->erase(socket);
				continue;
			}

			if (!message.empty())
				this->notify("message from client -> " + message);

			this->notify_everyone(socket, message);
		}
	}
}

void Server::notify(const std::string& message) const
{
	std::lock_guard<std::mutex> guard(mutex);

	std::cout << "Server: " + message << std::endl;
}

void Server::notify_everyone(const SOCKET& exceptSocket, const std::string& message) const
{
	std::lock_guard<std::mutex> guard(mutex);

	for (const auto& client : this->clients)
	{
		if (client != exceptSocket)
			this->server_send(client, message);
	}
}

bool Server::server_send(const SOCKET& socket, const std::string& message) const
{
	std::string msg = message;
	msg += '\0';

	char* ptr = (char*)(msg.c_str());
	int length = static_cast<int>(msg.length());

	while (length > 0)
	{
		int i = send(socket, ptr, length, 0);

		if (i == SOCKET_ERROR)
			return false;

		ptr += i;
		length -= i;
	}

	return true;
}

bool Server::server_recv(const SOCKET& socket, std::string& message) const
{
	const int max_buffer_size = Configuration::BUFFER_SIZE;

	char buffer[max_buffer_size];

	int size_recv;

	while (true)
	{
		memset(buffer, 0, max_buffer_size);

		if ((size_recv = recv(socket, buffer, max_buffer_size, 0) == SOCKET_ERROR))
		{
			return false;
		}
		else
		{
			if (message[message.size()] == '\0')
			{
				message += buffer;
				return true;
			}
		}
	}

	return true;
}

bool Server::is_client_connection_close(const std::string& message) const
{
	if (message.find(Configuration::EXIT_SYMBOL) != std::string::npos)
		return true;

	return false;
}

void Server::erase(const SOCKET& socket)
{
	std::lock_guard<std::mutex> guard(mutex);

	std::vector<SOCKET>::iterator socketRemove;

	socketRemove = std::find(this->clients.begin(), this->clients.end(), socket);

	if (socketRemove != this->clients.end())
	{
		closesocket(socket);
		this->clients.erase(socketRemove);
	}
}

void Server::push_back(const SOCKET& socket)
{
	std::lock_guard<std::mutex> guard(mutex);

	this->clients.push_back(socket);
}
