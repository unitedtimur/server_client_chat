#include "server.h"

int main(int argc, char** argv)
{
	Server server("127.0.0.1", "9090");
	server.start();

	system("pause");
	return 0;
}