#include "client.h"

int main(int argc, char** argv)
{
	// Инициализируем клиент
	Client server("127.0.0.1", "9090");
	server.start();

	system("pause");
	return 0;
}