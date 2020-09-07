#pragma once

#include <iostream>
#include <vector>

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

class Server
{
public:
	explicit Server(const std::string& host, const std::string& port);
	~Server();

	void start();

protected:
	void server_listen();
	void recv_info();
	void notify(const std::string& message) const;
	void notify_everyone(const SOCKET& exceptSocket, const std::string& message) const;

	bool server_send(const SOCKET& socket, const std::string& message) const;
	bool server_recv(const SOCKET& socket, std::string& message) const;

	bool is_client_connection_close(const std::string& message) const;

	void erase(const SOCKET& socket);
	void push_back(const SOCKET& socket);

private:
	mutable std::string host;
	mutable std::string port;
	
	SOCKET serverSocket;
	addrinfo* serverAddress;

	std::vector<SOCKET> clients;
};