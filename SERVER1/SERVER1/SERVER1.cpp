#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#pragma warning(disable: 4996)
using namespace std;
//Структура очереди буферов
struct que {
	char qu[50][256] = {};
	int back_i, front_i, n = 50;
public:
	que() {
		front_i = 1;
		back_i = 0;
	}
	//Добавление буфера с сообщением в очередь
	void push(char buf[256], int size) {
		if (back_i < n - 1) {
			for (int i = 0; i < size; ++i) {
				qu[back_i][i] = buf[i];
			}
			back_i++;
		}
		else {
			cout << "Queue overloaded" << endl;
		}
	}
	//проверка пустая ли очередь
	bool empty() {
		if (back_i < front_i) return 1;
		return 0;
	}
	//освобождение буфера из очереди
	void pop() {

		if (this->empty())cout << "Queue empty" << endl;
		else {
			for (int i = 0; i < back_i; ++i) {
				for (int j = 0; j < sizeof(qu[i]); ++j) {

					qu[i][j] = qu[i + 1][j];
				}
			}
			back_i--;
		}
	}
};


condition_variable cv;
SOCKET sListen;
fd_set master;
int fd_max;
mutex mtx, mtx1;
bool g_d;
que buf;
queue<int> address;


// Функция обработки буфера и отправки сообщения другим клиентам
void ClientHandler() {

	


	while (true) {
		
		// Поток засыпает в ожидании пополнения очереди
		if (buf.empty() || address.empty()) {
			unique_lock<mutex> lk(mtx);
			while (!g_d) // От ложных срабатываний
				cv.wait(lk);
		}
		//Создание копий
		fd_set read_fds;
		FD_ZERO(&read_fds);
		int fd_max_temp, add,sL_temp;
		que buf_temp;
		{
			lock_guard<mutex> lk1(mtx1);
			read_fds = master;
			fd_max_temp = fd_max;
			buf_temp = buf;
			add = address.front();
			sL_temp = sListen;
		}
		
		
		// Пересыл сообщения все клиентам, кроме отправителя
		if (FD_ISSET(add, &read_fds)) {
			for (int j = 0; j <= fd_max_temp; j++) {
				if (FD_ISSET(j, &read_fds)) {
					if (j != add && j != sL_temp) {

						send(j, buf_temp.qu[0], sizeof(buf_temp.qu[0]), NULL);

					}
				}
			}
			{
				lock_guard<mutex> lk1(mtx1);
				address.pop();
				buf.pop(); 
			}
			
			g_d = false;
		}

	}

}
//Функция чтения присланных сообщений,также закрывает оборванные соединения
void reading(int i) {
	char msg[256];
	if (recv(i, msg, sizeof(msg), NULL) <= 0) {

		shutdown(i, 2);
		closesocket(i);
		FD_CLR(i, &master);
	}
	else {
		lock_guard<mutex> lk(mtx);
		buf.push(msg,sizeof(msg));
		address.push(i);
		g_d = true;
		cv.notify_one();
	}
	
}
//Настройка слушающего сокета
void setting_Lis(SOCKADDR_IN addr) {
	sListen = socket(AF_INET, SOCK_STREAM, NULL);
	if (sListen == INVALID_SOCKET) {
		cout << "ERROR!!!" << endl;
		exit(1);
	}
	if (bind(sListen, (SOCKADDR*)& addr, sizeof(addr)) == SOCKET_ERROR) {
		cout << "ERROR" << endl;
		shutdown(sListen, 2);
		closesocket(sListen);
		exit(1);
	}
	if (listen(sListen, SOMAXCONN) != 0) {
		cout << "ERROR" << endl;
		shutdown(sListen, 2);
		closesocket(sListen);
		exit(1);
	}
}
//Подключение новых клиентов
void Accepting(int i, SOCKADDR_IN addr, int sizeofaddr) {
	SOCKET newConnection;
	{
		lock_guard<mutex> lk1(mtx1);
		newConnection = accept(sListen, (SOCKADDR*)& addr, &sizeofaddr);
	}
	if (newConnection == -1) {
		perror("accept");
	}
	else {
		{
			lock_guard<mutex> lk1(mtx1);
			FD_SET(newConnection, &master);
			if (newConnection > fd_max) {
				fd_max = newConnection;
			}
		}
		cout << "Client connected!" << endl;
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
// Основная функция
int main(int argc, char* argv[]) {

	librun();
	//Настройка адреса
	SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(addr));
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	setting_Lis(addr);
	FD_ZERO(&master);
	FD_SET(sListen, &master);
	fd_max = sListen;
	int fd_max_temp = fd_max;
	thread thread(ClientHandler);
	fd_set read_fds;
	FD_ZERO(&read_fds);
	int i;
	//Цикл первого потока
	while (true) {
		{
			lock_guard<mutex> lk1(mtx1);
			read_fds = master;
		}

		if (select(fd_max_temp + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			return 1;
		}
		//Обработка набора дескрипторов
		for (i = 0; i <= fd_max_temp; ++i) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == sListen) {
					Accepting(i, addr, sizeofaddr);
					{
						lock_guard<mutex> lk1(mtx1);
						fd_max_temp = fd_max;
					}
				}
				else {
					reading(i);
				}
			}
		}
	}
	shutdown(sListen, 2);
	closesocket(sListen);
	thread.join();
	system("pause");
	return 0;
}