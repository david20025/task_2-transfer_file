#pragma once
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
#include <string.h>

typedef unsigned short int usi_t;
typedef const socklen_t csl_t;
typedef socklen_t sl_t;
typedef int s_t;
typedef sockaddr_in sai_t;
typedef sockaddr sa_t;
typedef addrinfo ai_t;
typedef unsigned long long ull_t;

typedef unsigned short int usi_t;
typedef int s_t;

usi_t find_open_port();

s_t get_listen_socket(usi_t num_port);

s_t try_accept(s_t sd);

void accept_for_thread(usi_t thread_num);

void get_part(size_t num, ull_t fsize, ull_t part, void* mapped_file);

void Send(std::string& adress, std::string& path, size_t count_threads);

void Receive();

void send_part(size_t num, ull_t fsize, ull_t part, void* mapped_file);