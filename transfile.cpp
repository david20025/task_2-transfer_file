#include<iostream>
#include<string>


#if defined(_WIN32)
#include"win_api.h" 
#pragma comment(lib, "ws2_32.lib")
#elif defined(__linux)
#include"unix_api.h"
#endif



int main(int argc, char *argv[]) {
#if defined(_WIN32)
	WSADATA wsa_data;
	int err_code = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (err_code!=0) {
		std::cout << "WSAstartup error code " << err_code << std::endl;
		return 0;
	}
#endif
	if (argc == 2) {
		Receive();
	}
	else {
		std::string adress = argv[3], fname = argv[5], count = argv[7];
		Send(adress, fname, stoi(count));
	}
#if defined(_WIN32) || defined(_WIN64)
	WSACleanup();
#endif
	return 0;
}