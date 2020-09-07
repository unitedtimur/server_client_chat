#pragma once

#include <iostream>
#include <vector>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

class Client
{
public:
	explicit Client(const std::string& host, const std::string& port);
	~Client();

	void start();

protected:
	void client_listen();
	void client_chat();
	void notify(const std::string& message) const;

	bool client_send(const std::string& message) const;
	bool client_recv(std::string& message) const;

private:
	mutable std::string host;
	mutable std::string port;

	SOCKET clientSocket;
	addrinfo* addr;
};