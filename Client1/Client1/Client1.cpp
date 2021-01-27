#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>

#pragma warning(disable: 4996)
using namespace std;
SOCKET Connection;
std::condition_variable cv;
std::mutex mtx;
bool g_d = false;

// Принятие сообщений и вывод в консоль
void ClientHandler() {

	
	while (true) {
		char msg[256] = {};
			if (recv(Connection, msg, sizeof(msg), NULL) <= 0) {
				std::cout << "connection broken" << std::endl;
				system("pause");
				exit(1);

			}
		
		std::cout << msg << std::endl;
		
		
		
		
	}
}
// Запуск библиотеки
void librun() {
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}
}
//Функция подключения к серверу
int Connection_fun(SOCKADDR_IN addr){
	Connection = socket(AF_INET, SOCK_STREAM, NULL);
	if (connect(Connection, (SOCKADDR*)& addr, sizeof(addr)) != 0) {
		std::cout << "Error: failed connect to server.\n";
		system("pause");
		return 1;
	}
	std::cout << "Connected!\n";
}
//Основная функция
int main(int argc, char* argv[]) {
	librun();
	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	if (Connection_fun(addr) == 1) {
		return 1;
	}
	
	
	std::thread thread(ClientHandler);
	
	char msg1[256];
	//Написание  и отправка сообщения
	while (true) {
		
		std::cin.getline(msg1, sizeof(msg1));
		send(Connection, msg1, sizeof(msg1), NULL);
		Sleep(10);
		
	}
	thread.join();
	system("pause");
	return 0;
}
