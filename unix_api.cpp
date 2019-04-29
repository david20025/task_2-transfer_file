#include"unix_api.h"
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <libgen.h>
#include <iostream>
#include <mutex>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <thread>
#include <vector>
#include <netdb.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libgen.h>
#include <string.h>

usi_t find_open_port() {
	for (usi_t num_port = 1999; num_port < USHRT_MAX; ++num_port) {
		s_t sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd < 0) {
			std::cout << "Error!!! Can't create socket" << std::endl;
			return 0;
		}
		char optval;
		const sl_t optlen = (sl_t) sizeof(optval);
		setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &optval, optlen); //��������� ������� ���������� ��������� �� ������ ������
		setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen); //��������� ����������������� ������ �� ������ ������
		sai_t reciever;
		reciever.sin_family = AF_INET; //��� ������ � ipv4
		reciever.sin_port = htons(num_port); //������������� ����
		reciever.sin_addr.s_addr = INADDR_ANY;
		if (bind(sd, (sa_t *)&reciever, sizeof(sai_t)) < 0) { //�������� ��������� ����� � ������
			continue;
		}
		close(sd);
		return num_port;
	}
	return 0;
}

int get_listen_socket(usi_t num_port) {
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket < 0) {
		std::cout << "Error!!! Can't create socket" << std::endl;
		return 0;
	}
	sai_t reciever;
	reciever.sin_family = AF_INET; //��� ������ � ipv4
	reciever.sin_port = htons(num_port); //������������� ����
	reciever.sin_addr.s_addr = INADDR_ANY;
	char optval;
	sl_t optlen = (sl_t) sizeof(int);
	setsockopt(listen_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, optlen); //��������� ������� ���������� ��������� �� ������ ������ 
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, optlen); //��������� ����������������� ������ �� ������ ������
	if (bind(listen_socket, (sa_t *)&reciever, sizeof(sai_t)) < 0) {
		std::cout << "Error!!! Can't link adress with socket" << std::endl;
		return 0;
	}
	if (listen(listen_socket, SOMAXCONN) < 0) { //�������� ���������� � ������������ ���-��� ����������
		std::cout << "Error!!! Can't listen socket" << std::endl;
		close(listen_socket);
		return 0;
	}
	return listen_socket;
}

int get_connection(const char* node, usi_t num_port) {
	ai_t* result = NULL;
	ai_t info;
	info.ai_family = AF_INET; //ipv4
	info.ai_socktype = SOCK_STREAM; //��������� ��� ������
	info.ai_protocol = IPPROTO_TCP; //TCP ��������
	//info.ai_flags = AI_PASSIVE;
	char cur_port[7];
	//ZeroMemory(&port_buf, sizeof(port_buf));
    std::cout << cur_port <<" "<< node <<  std::endl;
	sprintf(cur_port, "%hu", num_port);
	if (getaddrinfo(node, cur_port, &info, &result) != 0) {
		std::cout << "Error!!! Can't get adress information." << std::endl;
		return -1;
	}
	s_t send_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (send_socket < 0) {
		std::cout << "Error!!! Can't create socket" << std::endl;
		freeaddrinfo(result);
		return -1;
	}
	if (connect(send_socket, result->ai_addr, (int)result->ai_addrlen) < 0) { //������������� ����������
		std::cout << "Error!!! Can't connect socket" << std::endl;
		close(send_socket);
		freeaddrinfo(result);
		return -1;
	}
	freeaddrinfo(result);
	return send_socket;
}

int
try_accept(s_t sd) {
	sai_t sender;
	sl_t size = sizeof(sai_t);
	s_t sender_socket = accept((s_t)sd,(sa_t *)&sender,&size);
	if (sender_socket < 0) {
		std::cout << "Error!!! Can't accept socket" << std::endl;
		return 0;
	}
	return sender_socket;
}

void
close_sockrfd(int fd) {
	close(fd);
}



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

namespace RECIEVE_SYNC {
	static std::mutex mutex_;
	static std::vector<s_t> listen_sockets, senders;
	static std::vector<std::thread> threads_acc;
	static std::vector<std::thread> threads_recv;
	static size_t count_threads;
}

void accept_for_thread(usi_t thread_num) {
	std::lock_guard<std::mutex> lock(RECIEVE_SYNC::mutex_);
	RECIEVE_SYNC::senders.emplace_back(try_accept(RECIEVE_SYNC::listen_sockets[thread_num]));
}

void get_part(size_t num, ull_t fsize, ull_t part, void* mapped_file) {
	std::lock_guard<std::mutex> lock(RECIEVE_SYNC::mutex_);
	ull_t part_with_info = (part) / 4096;
	if (part/4096==0) part_with_info=1;
	else if (part%4096!=0) part_with_info++;
	if (num == RECIEVE_SYNC::count_threads - 1) {
		auto remain = fsize - part * (RECIEVE_SYNC::count_threads - 1);
		part_with_info = (remain) / 4096;
		if (remain/4096==0) part_with_info=1;
		else if (remain%4096!=0) part_with_info++;
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
	char *bufn = new char[4096];
	char* bufs= new char[sizeof(ull_t)];
	char* bufc= new char[sizeof(size_t)];
	recv(sender, (char*)bufc, sizeof(size_t), 0);
	recv(sender, (char*)bufs, sizeof(ull_t), 0);
	recv(sender, (char*)bufn, 4096, 0);
	Ans buf_ans{};
	size_t count_threads = 1;
	strncpy((char *)(&count_threads), bufc, sizeof(size_t));
	RECIEVE_SYNC::count_threads = count_threads;
	strncpy((char *)(&fsize), bufs, sizeof(ull_t));
	std::string fname = bufn;
	buf_ans.count_ports = count_threads;
	for (usi_t i = 0; i < count_threads; ++i) {
		buf_ans.opened_ports[i] = find_open_port();
		RECIEVE_SYNC::listen_sockets.push_back(get_listen_socket(buf_ans.opened_ports[i]));
		RECIEVE_SYNC::threads_acc.push_back(std::thread(accept_for_thread, i));
	}
	send(sender, (const char *)(&buf_ans), sizeof(buf_ans), 0);
	fflush(stdout);
	int file = open(fname.c_str(), O_CREAT | O_RDWR | O_TRUNC, NULL); //������� ����
	ftruncate(file, fsize); //������� ����
	void* mapped_file = mmap(NULL, fsize, PROT_WRITE | PROT_READ, MAP_SHARED, file, 0);//���������� � ������
	close(file); //��������� ����
	ull_t part = (fsize) / count_threads;
	if (fsize/count_threads==0) part=1;
	else if (fsize%count_threads!=0) part++;
	//�������� ���� �������
	for (auto &acc : RECIEVE_SYNC::threads_acc) {
		acc.join();
	}
	for (usi_t i = 0; i < RECIEVE_SYNC::count_threads; ++i) {
		RECIEVE_SYNC::threads_recv.push_back(std::thread(get_part, i, fsize, part, mapped_file));
	}
	for (auto &recv : RECIEVE_SYNC::threads_recv) {
		recv.join();
	}

	for (size_t i = 0; i < RECIEVE_SYNC::listen_sockets.size(); ++i) {
		close(RECIEVE_SYNC::listen_sockets[i]);
		close(RECIEVE_SYNC::senders[i]);
	}
	close(listen_socket);
	close(sender);
	munmap(mapped_file, fsize);//������� �����������
	delete[] bufs;
	delete[] bufc;
	delete[] bufn;
}

namespace SEND_SYNC {
	static std::mutex mutex_;
	static std::vector<s_t> send_sockets;
	static std::vector<std::thread> threads_send;
	static usi_t count_threads;
	static std::string adress;
}

void send_part(size_t num, ull_t fsize, ull_t part, void* mapped_file) {
	std::lock_guard<std::mutex> lock(SEND_SYNC::mutex_);
	ull_t parts = (part) / 4096;
	if (part/4096==0) parts=1;
	else if (part%4096!=0) parts++;
	if (num == SEND_SYNC::count_threads - 1) {
		auto remain = fsize - part * (RECIEVE_SYNC::count_threads - 1);
		parts = (remain) / 4096;
		if (remain/4096==0) parts=1;
		else if (remain%4096!=0) parts++;
	}
	for (ull_t i = 0; i < parts; ++i) {
		ull_t size=4096;
		char* part_file = new char[4096];
		if (i == parts - 1) {
			if (num == RECIEVE_SYNC::count_threads - 1) {
				size = fsize - part * num - 4096 * i;
			}
			else {
				size = part - 4096 * i;
			}
		}
		memcpy(part_file, (char *)mapped_file + num * part + i * 4096, size);
		send(SEND_SYNC::send_sockets[num], part_file, size, 0);
		delete[] part_file;
	}
}

void Send(std::string& adress, std::string& path, size_t count_threads) {
	char* temp = strdup(path.c_str());
	if (strcmp(basename(temp), path.c_str()) == 0) {
		path = "./" + path;
	}
	int file = open(path.c_str(),O_RDONLY | O_WRONLY, NULL); //��������� ����
	struct stat ls;
	fstat(file, &ls);
	ull_t fsize = ls.st_size;
	void* mapped_file = mmap(NULL, fsize, PROT_READ, MAP_SHARED, file, 0);
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
	char* bufs= new char[sizeof(ull_t)];
	char* bufc= new char[sizeof(size_t)];
	strncpy(bufc, (char *)(&count_threads), sizeof(count_threads));
	strncpy(bufs, (char*)(&fsize), sizeof(ull_t));
	memset(bufn, '\0', 4096);
	char *temp = strdup(path.c_str());
	memcpy(bufn, basename(temp), 4096);
	send((s_t)sender, bufc, sizeof(size_t), 0);
	send((s_t)sender, bufs, sizeof(ull_t), 0);
	send((s_t)sender, bufn, 4096, 0);
	Ans buf_ans;
	recv((s_t)sender, (char *)(&buf_ans), sizeof(buf_ans), 0);
	count_threads = buf_ans.count_ports;
	SEND_SYNC::count_threads = count_threads;
	for (size_t i = 0; i < count_threads; ++i) {
		opened_ports.push_back(buf_ans.opened_ports[i]);
	}
	ull_t part = (fsize) / count_threads;
	if (fsize/count_threads==0) part=1;
	else if (fsize%count_threads!=0) part++;
	for (size_t i = 0; i < count_threads; ++i) {
		SEND_SYNC::send_sockets.push_back(get_connection(adress.c_str(), opened_ports[i]));
	}

	for (size_t i = 0; i < SEND_SYNC::count_threads; ++i) {
		SEND_SYNC::threads_send.push_back(std::thread(send_part, i, fsize, part, mapped_file));
	}
	for (auto &send : SEND_SYNC::threads_send) {
		send.join();
	}
	for (auto send_socket : SEND_SYNC::send_sockets) {
		close(send_socket);
	}
	close(sender);
	munmap(mapped_file, fsize);
	delete[] bufn;
	delete[] bufc;
	delete[] bufs;
	close(file);
}