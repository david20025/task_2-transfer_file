#pragma once
#include <atomic>
#include <fcntl.h>
#include <iostream>
#include <io.h>
#include <limits>
#include <mutex>
#include <stdio.h>
//#include <shlwapi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <vector>
#include <cstddef>
#include <cstring>
#include <utility>
//#include <inttypes.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

typedef unsigned short int usi_t;
typedef SOCKET s_t;
typedef unsigned long long ull_t;

usi_t find_open_port();

s_t get_listen_socket(usi_t num_port);

s_t try_accept(s_t sd);

void accept_for_thread(usi_t thread_num);

void get_part(size_t num, ull_t fsize, ull_t part, LPVOID mapped_file);

void Receive();

struct inform {
	size_t num;
	ull_t fsize;
	ull_t part;
	LPVOID mapped_file;
};

void send_part(inform  inf);


void Send(std::string& adress, std::string& path, size_t count_threads);