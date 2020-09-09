#include "server.h"
#include "../src/configuration.h"
#include "WS2tcpip.h"

#include <thread>
#include <mutex>
#include <sstream>

// Мьютекс для блокировки потоков
std::mutex mutex;

Server::Server(const std::string& host, const std::string& port) : 
	host(host),
	port(port),
	serverAddress(nullptr)
{
}

Server::~Server()
{
	// Закрываем сокет сервера
	closesocket(this->serverSocket);

	// Закрываем сокеты клиентов
	for (const auto& client : this->clients)
		closesocket(client);

	// Очищаем информацию сервера сокета
	freeaddrinfo(serverAddress);

	// Очистка запущенных данных из .dll
	WSACleanup();
}

void Server::start()
{
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);

	// Инициализируем WSAStartup и выставляем версию .dll
	if (WSAStartup(version, &wsaData) != 0)
	{
		// Если ошибка выводим информацию
		this->notify(Configuration::Server::ERROR_WSADATA);
		return;
	}

	addrinfo hints;
	
	// Инициализируем нулями hints
	ZeroMemory(&hints, sizeof(hints));

	// Устанавливаем ipv4
	hints.ai_family = AF_INET;

	// Для tcp SOCK_STREAM
	hints.ai_socktype = SOCK_STREAM;

	// Устанавливаем протокол
	hints.ai_protocol = IPPROTO_TCP;

	// Флаг для hints
	hints.ai_flags = AI_PASSIVE;

	// По полученному хосту, порту и hints, выставляем информацию для сервера
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &serverAddress) != 0)
	{
		// Если ошибка выводим информацию
		this->notify(Configuration::Server::ERROR_GETADDRINFO);
		return;
	}

	if ((this->serverSocket = socket(serverAddress->ai_family, serverAddress->ai_socktype, serverAddress->ai_protocol)) == INVALID_SOCKET)
	{
		// Если ошибка выводим информацию
		this->notify(Configuration::Server::ERROR_SOCKET);
		return;
	}

	if (bind(this->serverSocket, serverAddress->ai_addr, int(serverAddress->ai_addrlen)) == SOCKET_ERROR)
	{
		// Если ошибка выводим информацию
		this->notify(Configuration::Server::ERROR_BIND);
		return;
	}

	if (listen(this->serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		// Если ошибка выводим информацию
		this->notify(Configuration::Server::ERROR_LISTEN);
		return;
	}

	// Уведомляем о старте сервера
	this->notify(Configuration::Server::START);

	// Запуск потока, который ждёт клиентов
	std::thread threadListen(&Server::server_listen, this);

	// Запуск потока, который слушает клиентов
	std::thread threadRecv(&Server::recv_info, this);
	
	// Разраваем поток и не ждём его
	threadRecv.detach();

	// Ждём поток
	threadListen.join();
}

void Server::server_listen()
{
	// Уведомляем о том, что слушаем сокеты подключения
	this->notify(Configuration::Server::LISTEN + ' ' + this->port);

	std::string response = Configuration::COPYRIGHT;

	for (;;)
	{
		// Делаем маленькую паузу для потока
		std::this_thread::sleep_for(std::chrono::milliseconds(75));

		SOCKET clientSocket;

		// Принимаем сокет, и если что-то не так с подключением, то уведомляем об этом
		if ((clientSocket = accept(this->serverSocket, 0, 0)) == INVALID_SOCKET)
		{
			this->notify(Configuration::Server::ERROR_ACCEPT_FAILED);
			closesocket(clientSocket);
			continue;
		}

		// Отправляем клиенту ответ
		if (!this->server_send(clientSocket, response))
		{
			this->notify(Configuration::Server::ERROR_SEND);
			closesocket(clientSocket);
			continue;
		}

		// Уведомляем о новом подключении
		this->notify(Configuration::Server::NEW_CONNECTION);

		// Вносим в массив клиентов
		this->push_back(clientSocket);
	}
}

void Server::recv_info()
{
	// Уведомляем о прослушке, существующих сокетов
	this->notify(Configuration::Server::RECV);

	for (;;)
	{
		// Делаем маленькую паузу
		std::this_thread::sleep_for(std::chrono::milliseconds(75));

		// Получаем массив сокетов
		const auto sockets = this->clients;

		// Проходимся по каждому сокету
		for (const auto& socket : sockets)
		{
			std::string message; 

			// Слушаем сокет, и если что-то не так, удаляем его из массива сокета и говорим, о его отключении
			if (!this->server_recv(socket, message))
			{
				this->notify(Configuration::Server::CLIENT_DISCONNECT);
				this->erase(socket);
				continue;
			}
			
			// Если пользователь ввёл команду отключения
			if (this->is_client_connection_close(message))
			{
				this->notify(Configuration::Server::CLIENT_DISCONNECT);
				this->erase(socket);
				continue;
			}

			// Если пришло не пустое сообщение, то выводим это
			if (!message.empty())
			{
				this->notify("message from client -> " + message);

				// Уведомляем всех клиентов о собщении
				this->notify_everyone(socket, message);
			}
		}
	}
}

void Server::notify(const std::string& message) const
{
	// Блокируем текущий блок функции для потока
	std::lock_guard<std::mutex> guard(mutex);

	// Выводим информацию
	std::cout << "Server: " + message << std::endl;
}

void Server::notify_everyone(const SOCKET& exceptSocket, const std::string& message) const
{
	// Блокируем текущий блок функции для потока
	std::lock_guard<std::mutex> guard(mutex);

	// Проходимся по всем сокетам и отправляем информацию
	for (const auto& client : this->clients)
	{
		if (client != exceptSocket)
			this->server_send(client, message);
	}
}

bool Server::server_send(const SOCKET& socket, const std::string& message) const
{
	// Добавляем признак конца строки
	std::string msg = message;
	msg += '\0';

	// Кастуем к char* 
	char* ptr = (char*)(msg.c_str());

	// Получаем длину строки
	int length = static_cast<int>(msg.length());

	// До тех пор пока не отправим всю строку
	while (length > 0)
	{
		// Отправляем сокету строку с длинной
		int i = send(socket, ptr, length, 0);

		// Если во время отправки что-то случится - вернём false
		if (i == SOCKET_ERROR)
			return false;

		// Добавляем количество символов
		ptr += i;

		// Уменьшаем длину строки
		length -= i;
	}

	// Если всё ок, возвращаем true
	return true;
}

bool Server::server_recv(const SOCKET& socket, std::string& message) const
{
	// Получаем максимальный буффер строки
	const int max_buffer_size = Configuration::BUFFER_SIZE;

	// Делаем char буффер
	char buffer[max_buffer_size];

	int size_recv;

	// Работаем пока не получим всю информацию
	while (true)
	{
		// Очищаем буффер
		memset(buffer, 0, max_buffer_size);

		// Получаем строку, если что-то не так сразу выйдем с false
		if ((size_recv = recv(socket, buffer, max_buffer_size, 0) == SOCKET_ERROR))
		{
			return false;
		}
		else
		{
			// Если дошли до конца строки, выходим успешно
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
	// Если в строке есть символ выхода, то вернём true
	if (message.find(Configuration::EXIT_SYMBOL) != std::string::npos)
		return true;

	return false;
}

void Server::erase(const SOCKET& socket)
{
	// Блокируем текущий блок функции для потока
	std::lock_guard<std::mutex> guard(mutex);

	std::vector<SOCKET>::iterator socketRemove;

	// Ищу сокет в массиве
	socketRemove = std::find(this->clients.begin(), this->clients.end(), socket);

	// Если итератор не найден (такой сокет)
	if (socketRemove != this->clients.end())
	{
		// Закрываем сокет
		closesocket(socket);

		// Удаляем из массива сокет
		this->clients.erase(socketRemove);
	}
}

void Server::push_back(const SOCKET& socket)
{
	// Блокируем блок функции для потока
	std::lock_guard<std::mutex> guard(mutex);

	// Помещаем клиента в массив клиентов
	this->clients.push_back(socket);
}
