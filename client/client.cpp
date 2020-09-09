#include "client.h"
#include "../src/configuration.h"
#include "WS2tcpip.h"

#include <thread>
#include <mutex>
#include <string>

// Мьютекс, блокирующий потоки
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
	// Закрываем сокет
	closesocket(this->clientSocket);
	// Очищаем информацию и WSACleanup
	freeaddrinfo(addr);
	WSACleanup();
}

void Client::client_listen()
{
	// Уведомляем о прослушивании
	this->notify(Configuration::Client::LISTEN + '\n');

	for (;;)
	{
		// Делаем задержку у потока
		std::this_thread::sleep_for(std::chrono::milliseconds(75));

		std::string message;

		// Получаем информацию от сервера
		if (!this->client_recv(message))
		{
			this->notify(Configuration::Server::ERROR_RECV + '\n');
			return;
		}

		// Если сообщение не пустое, то выводим это
		if (!message.empty())
			this->notify("message from server -> " + message + '\n');
	}
}

void Client::client_chat()
{
	// Делаем паузу для потока
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::string message;

	// В бесконечном цикле можем вводить информацию в консоль и отправлять её
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(75));
		
		// Приглашаем ввести сообщение
		this->notify("Send -> ");

		// Считываем строку
		std::getline(std::cin, message);

		// Если строка не пустая
		if (!message.empty())
			this->client_send(message);

		// Очищаем сообщение
		message.clear();
	}
}

void Client::start()
{
	// Инициализация аналогична серверу
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);

	if (WSAStartup(version, &wsaData) != 0)
	{
		this->notify(Configuration::Client::ERROR_WSADATA);
		WSACleanup();
		return;
	}

	addrinfo hints;

	// Аналогично серверу
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Аналогично серверу
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addr) != 0)
	{
		this->notify(Configuration::Client::ERROR_GETADDRINFO + '\n');
		return;
	}

	// Аналогично серверу
	if ((this->clientSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == INVALID_SOCKET)
	{
		this->notify(Configuration::Client::ERROR_SOCKET + '\n');
		return;
	}

	// Данная функция преобразует строку символов addr->ai_addr->sa_data в структуру сетевого адреса сетевого семейства адресов
	if (inet_pton(addr->ai_family, addr->ai_addr->sa_data, (PVOID)(addr->ai_addrlen)) == SOCKET_ERROR)
	{
		this->notify(Configuration::Client::INVALID_HOST + '\n');
		return;
	}

	// Производим подключение к сокету сервера
	if (connect(this->clientSocket, addr->ai_addr, int(addr->ai_addrlen)) == SOCKET_ERROR)
	{
		this->notify(Configuration::Client::ERROR_CONNECT + '\n');
		return;
	}

	// Уведомляем о запуске клиента
	this->notify(Configuration::Client::START + '\n');

	// Запускаем поток слушащий сокет сервера
	std::thread(&Client::client_listen, this).detach();

	// Запускаем поток ждущий ввод строк
	std::thread(&Client::client_chat, this).join();
}

void Client::notify(const std::string& message) const
{
	// Блокируем поток функции
	std::lock_guard<std::mutex> guard(mutex);

	// Выводим информацию
	std::cout << "Client: " + message << std::flush;
}

bool Client::client_send(const std::string& message) const
{
	// Аналогично серверу
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
	// Аналогично серверу
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
