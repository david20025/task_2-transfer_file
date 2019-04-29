#define _CRT_SECURE_NO_WARNINGS
#include"win_api.h"
#include<limits>


typedef unsigned short int usi_t;
typedef const socklen_t csl_t;
typedef socklen_t sl_t;
typedef SOCKET s_t;
typedef SOCKADDR_IN sai_t;
typedef sockaddr sa_t;
typedef addrinfo ai_t;
typedef unsigned long long ull_t;


usi_t find_open_port() {
	for (usi_t num_port = 1999; num_port < USHRT_MAX; ++num_port) {
		s_t sd = socket(AF_INET, SOCK_STREAM, 0); //создаем сокет
		if (sd < 0) {
			std::cout << "Error!!! Can't create socket" << std::endl;
			return 0;
		}
		int optval = 0;
		csl_t optlen = (sl_t)sizeof(int);
		setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, optlen); //разрешаем посылку оживляющих сообщений на уровне сокета
		setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, optlen); //разрешаем переиспользование сокета на уровне сокета
		sai_t reciever;
		reciever.sin_family = AF_INET; //для работы с ipv4
		reciever.sin_port = htons(num_port); //устанавливаем порт
		reciever.sin_addr.s_addr = INADDR_ANY;
		if (bind(sd, (sa_t *) &reciever, sizeof(sai_t)) < 0) { //пытаемся подвязать адрес к сокету
			continue;
		}
		closesocket(sd);
		return num_port;
	}
	return 0;
}


s_t get_listen_socket(usi_t num_port) {
	PADDRINFOA result = NULL;
	ADDRINFOA info;
	ZeroMemory(&info, sizeof(info));
	info.ai_family = AF_INET; //ipv4
	info.ai_socktype = SOCK_STREAM; //потоковый тип сокета
	info.ai_protocol = IPPROTO_TCP; //TCP протокол
	info.ai_flags = AI_PASSIVE;
	char cur_port[7];
	sprintf(cur_port, "%hu", num_port);
	if (GetAddrInfoA(NULL, cur_port, &info, &result) != 0) {
		std::cout << "Error!!! Can't get adress information." << std::endl;
		return 0;
	}
	s_t listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket <0) {
		std::cout << "Error!!! Can't create socket" << std::endl;
		freeaddrinfo(result);
		return 0;
	}
	int optval = 0;
	csl_t optlen = (sl_t)sizeof(int);
	setsockopt(listen_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, optlen); //разрешаем посылку оживляющих сообщений на уровне сокета 
	setsockopt(listen_socket,SOL_SOCKET,SO_REUSEADDR, (char*)&optval,optlen); //разрешаем переиспользование сокета на уровне сокета
	if (bind(listen_socket, result->ai_addr, (int)result->ai_addrlen)<0) { //пытаемся подвязать адрес к сокету
		std::cout << "Error!!! Can't link adress with socket" << std::endl;
		freeaddrinfo(result);
		closesocket(listen_socket);
		return 0;
	}
	if (listen(listen_socket, SOMAXCONN)<0) { //пытаемся прослушить с максимальным кол-вом соединений
		std::cout << "Error!!! Can't listen socket" << std::endl;
		closesocket(listen_socket);
		freeaddrinfo(result);
		return 0;
	}
	freeaddrinfo(result);
	return listen_socket;
}

int get_connection(const char* node,usi_t num_port) {
	PADDRINFOA result = NULL;
	ADDRINFOA info;
	ZeroMemory(&info, sizeof(info));
	info.ai_family = AF_INET; //ipv4
	info.ai_socktype = SOCK_STREAM; //потоковый тип сокета
	info.ai_protocol = IPPROTO_TCP; //TCP протокол
	char cur_port[7];
	sprintf(cur_port, "%hu", num_port);
	if (GetAddrInfoA(node, cur_port, &info, &result) != 0) {
		std::cout << "Error!!! Can't get adress information." << std::endl;
		return -1;
	}
	s_t send_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (send_socket < 0) {
		std::cout << "Error!!! Can't create socket" << std::endl;
		freeaddrinfo(result);
		return -1;
	}
	if (connect(send_socket, result->ai_addr, (int)result->ai_addrlen) < 0) { //устанавливаем соединение
		std::cout << "Error!!! Can't connect socket" << std::endl;
		closesocket(send_socket);
		freeaddrinfo(result);
		return -1;
	}
	freeaddrinfo(result);
	return send_socket;
}

s_t try_accept(s_t sd) {
	sai_t sender;
	sl_t size = sizeof(sai_t);
	s_t sender_socket = accept((s_t)sd,(sa_t *) &sender,&size);
	if (sender_socket<0) {
		std::cout << "Error!!! Can't accept socket" << std::endl;
		return 0;
	}
	return sender_socket;
}

void
close_sockrfd(int fd) {
	closesocket((SOCKET)fd);
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

namespace RECIEVE_SYNC{
	static std::mutex mutex_;
	static std::vector<s_t> listen_sockets, senders;
	static std::vector<std::thread> threads_acc;
	static std::vector<std::thread> threads_recv;
	static usi_t count_threads;
}

void accept_for_thread(usi_t thread_num) {
	std::lock_guard<std::mutex> lock(RECIEVE_SYNC::mutex_);
	RECIEVE_SYNC::senders.emplace_back(try_accept(RECIEVE_SYNC::listen_sockets[thread_num]));
}

void get_part(size_t num, ull_t fsize, ull_t part, LPVOID mapped_file) {
	std::lock_guard<std::mutex> lock(RECIEVE_SYNC::mutex_);
	ull_t part_with_info = (part) / 4096;
	if (part / 4096 == 0) part_with_info = 1;
	else if (part % 4096 != 0) part_with_info++;
	if (num == RECIEVE_SYNC::count_threads - 1) {
		auto remain = fsize - part *(RECIEVE_SYNC::count_threads - 1);
		part_with_info = (remain) / 4096;
		if (remain / 4096 == 0) part_with_info = 1;
		else if (remain % 4096 != 0) part_with_info++;
	}
	for (ull_t i = 0; i < part_with_info; ++i) {
		ull_t size=4096;
		char* part_file = new char[4096];
		if (i == part_with_info - 1) {
			if (num == RECIEVE_SYNC::count_threads - 1) {
				size = fsize - part * num - 4096 * i;
			}
			else {
				size = part - 4096 * i;
			}
		}
		recv(RECIEVE_SYNC::senders[num], part_file, size, 0);
		memcpy((char *)mapped_file + num * part + i * 4096, part_file, size);
		delete[] part_file;
	}
}

struct Ans {
	usi_t count_ports;
	usi_t opened_ports[1000] = { 0 };
};

void Receive() {
	usi_t opened_port = find_open_port();
	s_t listen_socket = get_listen_socket(opened_port);
	if (listen_socket == 0) {
		return;
	}
	s_t sender = try_accept(listen_socket);
	ull_t fsize;
	fflush(stdout);
	char* bufn = new char[4096];
	char* bufs = new char[sizeof(ull_t)];
	char* bufc = new char[sizeof(size_t)];
	recv(sender, (char*)bufc,sizeof(size_t), 0);
	recv(sender, (char*)bufs,sizeof(ull_t), 0);
	recv(sender, (char*)bufn, 4096, 0);
	Ans buf_ans;
	usi_t count_threads = 1;
	strncpy((char *)(&count_threads), bufc, sizeof(size_t));
	RECIEVE_SYNC::count_threads = count_threads;
	strncpy((char*)(&fsize), bufs, sizeof(ull_t));
	std::string fname = bufn;
	buf_ans.count_ports = count_threads;
	for (usi_t i = 0; i < count_threads; ++i) {
		buf_ans.opened_ports[i] = find_open_port();
		RECIEVE_SYNC::listen_sockets.push_back(get_listen_socket(buf_ans.opened_ports[i]));
		RECIEVE_SYNC::threads_acc.push_back(std::thread(accept_for_thread, i));
	}
	send(sender, (const char *)(&buf_ans), sizeof(buf_ans), 0);
	fflush(stdout);
	HANDLE handle = CreateFileA(fname.c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL); //создаем handle
	int file = _open_osfhandle((intptr_t)handle, _O_APPEND | _O_RDONLY | _O_WRONLY | _O_TRUNC); //открываем файл
	_chsize_s(file, fsize); //усекаем файл
	HANDLE handle2 = CreateFileMapping(handle, NULL, PAGE_READWRITE, 0, 0, NULL);//подготавливаем файл
	LPVOID mapped_file = MapViewOfFile(handle2, FILE_MAP_WRITE, 0, 0, fsize);//отображаем в память
	ull_t part = (fsize) / count_threads;
	if (fsize / count_threads == 0) part = 1;
	else if (fsize % count_threads != 0) part++;
	for (auto &acc : RECIEVE_SYNC::threads_acc) {
		acc.join();
	}
	for (usi_t i = 0; i < RECIEVE_SYNC::count_threads; ++i) {
		RECIEVE_SYNC::threads_recv.push_back(std::thread(get_part, i, fsize, part, mapped_file));
	}
	for (auto &recv : RECIEVE_SYNC::threads_recv) {
		recv.join();
	}
	
	for (size_t i = 0; i < RECIEVE_SYNC::listen_sockets.size();++i) {
		closesocket(RECIEVE_SYNC::listen_sockets[i]);
		closesocket(RECIEVE_SYNC::senders[i]);
	}
	closesocket(listen_socket);
	closesocket(sender);
	UnmapViewOfFile(mapped_file);
	delete[] bufc;
	delete[] bufn;
	delete[] bufs;
	_close(file); //закрываем файл
}

namespace SEND_SYNC {
	static std::mutex mutex_;
	static std::vector<s_t> send_sockets;
	static std::vector<std::thread> threads_send;
	static usi_t count_threads;
	static std::string adress;
}


void send_part(inform  inf) {
	std::lock_guard<std::mutex> lock(SEND_SYNC::mutex_);
	ull_t parts = (inf.part) / 4096;
	if (inf.part / 4096 == 0) parts = 1;
	else if (inf.part % 4096 != 0) parts++;
	if (inf.num == SEND_SYNC::count_threads - 1) {
		auto remain = inf.fsize - inf.part * (SEND_SYNC::count_threads - 1);
		parts = (remain) / 4096;
		if (remain / 4096 == 0) parts = 1;
		else if (remain % 4096 != 0) parts++;
	}
	for (ull_t i = 0; i < parts; ++i) {
		char* part_file = new char[4096];
		ull_t size = 4096;
		if (i == parts - 1) {
			if (inf.num == RECIEVE_SYNC::count_threads - 1) {
				size = inf.fsize - inf.part * inf.num - 4096 * i;
			}
			else {
				size = inf.part - 4096 * i;
			}
		}
		memcpy(part_file, (char*)inf.mapped_file + inf.num * inf.part + i * 4096, size);
		send(SEND_SYNC::send_sockets[inf.num], part_file, size, 0);
		delete[] part_file;
	}
}


void Send(std::string& adress,std::string& path, size_t count_threads) {
	char fsname[4096];
	_splitpath((const char*)(path.c_str()), NULL, NULL, fsname, NULL);
	if (strcmp(fsname, path.c_str()) == 0) {
		path = "./" + path;
	}
	HANDLE handle = CreateFileA(path.c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL| FILE_FLAG_OVERLAPPED, NULL); //создаем handle
	int file = _open_osfhandle((intptr_t)handle, _O_APPEND | _O_RDONLY | _O_WRONLY | _O_TRUNC); //открываем файл
	ull_t fsize = _filelengthi64(file);
	HANDLE file_handle = (HANDLE)_get_osfhandle(file);
	HANDLE handle2 = CreateFileMapping(file_handle, NULL, PAGE_READWRITE, 0, 0, NULL);
	void* mapped_file = MapViewOfFile(handle2, FILE_MAP_READ, 0, 0, fsize);
	usi_t port;
	s_t sender;
	for (port = 1999; port < USHRT_MAX; ++port) {
		sender = get_connection(adress.c_str(), port);
		if (sender == -1) {
			continue;
		}
		break;
	}
	std::vector<usi_t> opened_ports;
	char* bufn = new char[4096];
	char* bufs = new char[sizeof(uint64_t)];
	char* bufc=new char[sizeof(size_t)];
	std::string sc = (char*)(&count_threads);
	strncpy(bufc, sc.c_str(), sizeof(count_threads));
	std::string ss = (char*)(&fsize);
	strncpy(bufs, ss.c_str(), sizeof(fsize));
	strncpy(bufn, fsname, 4096);
	send((s_t)sender, (char *)bufc, sizeof(size_t), 0);
	send((s_t)sender, (char*)bufs, sizeof(ull_t), 0);
	send((s_t)sender, (char*)bufn, 4096, 0);
	Ans buf_ans;
	recv((s_t)sender, (char *)(&buf_ans), sizeof(buf_ans),0);
	count_threads = buf_ans.count_ports;
	SEND_SYNC::count_threads = count_threads;
	for (size_t i = 0; i < count_threads; ++i) {
		opened_ports.push_back(buf_ans.opened_ports[i]);
	}
	ull_t part = (fsize) / count_threads;
	if (fsize / count_threads == 0) part = 1;
	else if (fsize % 4096 != 0) part++;
	for (size_t i = 0; i < count_threads; ++i) {
		SEND_SYNC::send_sockets.emplace_back(get_connection(adress.c_str(), opened_ports[i]));
	}
	inform inf;
	for (size_t i = 0; i < count_threads; ++i) {
		inf.mapped_file = mapped_file;
		inf.fsize = fsize;
		inf.num = i;
		inf.part = part;
		SEND_SYNC::threads_send.emplace_back(send_part, inf);
	}
	for (auto& send : SEND_SYNC::threads_send) {
		send.join();
	}
	for (auto send_socket : SEND_SYNC::send_sockets) {
		closesocket(send_socket);
	}
	closesocket(sender);
	UnmapViewOfFile(mapped_file);
	_close(file);
}
