#include "client.h"
#include "../src/configuration.h"
#include "WS2tcpip.h"

#include <thread>
#include <mutex>
#include <string>

std::mutex mutex;

Client::Client(const std::string& host, const std::string& port) :
	host(host),
	port(port),
	clientSocket(-1),
	addr(nullptr)
{
}

Client::~Client()
{
	closesocket(this->clientSocket);
	freeaddrinfo(addr);
	WSACleanup();
}

void Client::client_listen()
{
	this->notify(Configuration::Client::LISTEN + '\n');

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(75));

		std::string message;

		if (!this->client_recv(message))
		{
			this->notify(Configuration::Server::ERROR_RECV + '\n');
			return;
		}

		if (!message.empty())
			this->notify("message from server -> " + message + '\n');
	}
}

void Client::client_chat()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::string message;

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(75));
		
		this->notify("Send -> ");

		std::getline(std::cin, message);

		if (!message.empty())
			this->client_send(message);

		message.clear();
	}
}

void Client::start()
{
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);

	if (WSAStartup(version, &wsaData) != 0)
	{
		this->notify(Configuration::Client::ERROR_WSADATA);
		WSACleanup();
		return;
	}

	addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addr) != 0)
	{
		this->notify(Configuration::Client::ERROR_GETADDRINFO + '\n');
		return;
	}

	if ((this->clientSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == INVALID_SOCKET)
	{
		this->notify(Configuration::Client::ERROR_SOCKET + '\n');
		return;
	}

	if (inet_pton(addr->ai_family, addr->ai_addr->sa_data, (PVOID)(addr->ai_addrlen)) == SOCKET_ERROR)
	{
		this->notify(Configuration::Client::INVALID_HOST + '\n');
		return;
	}

	if (connect(this->clientSocket, addr->ai_addr, int(addr->ai_addrlen)) == SOCKET_ERROR)
	{
		this->notify(Configuration::Client::ERROR_CONNECT + '\n');
		return;
	}

	this->notify(Configuration::Client::START + '\n');

	std::thread(&Client::client_listen, this).detach();
	std::thread(&Client::client_chat, this).join();
}

void Client::notify(const std::string& message) const
{
	std::lock_guard<std::mutex> guard(mutex);

	std::cout << "Client: " + message << std::flush;
}

bool Client::client_send(const std::string& message) const
{
	std::string msg = message;
	msg += '\0';

	char* ptr = (char*)(msg.c_str());
	int length = static_cast<int>(msg.length());

	while (length > 0)
	{
		int i = send(this->clientSocket, ptr, length, 0);

		if (i == SOCKET_ERROR)
			return false;

		ptr += i;
		length -= i;
	}

	return true;
}

bool Client::client_recv(std::string& message) const
{
	const int max_buffer_size = Configuration::BUFFER_SIZE;

	char buffer[max_buffer_size];

	int size_recv;

	while (true)
	{
		memset(buffer, 0, max_buffer_size);

		if ((size_recv = recv(this->clientSocket, buffer, max_buffer_size, 0) == SOCKET_ERROR))
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
