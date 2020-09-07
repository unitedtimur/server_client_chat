#include "client.h"

int main(int argc, char** argv)
{
	Client server("127.0.0.1", "9090");
	server.start();

	system("pause");
	return 0;
}