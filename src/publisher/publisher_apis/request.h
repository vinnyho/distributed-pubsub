#include <arpa/inet.h>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <netinet/in.h>
#include <set>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

static size_t WriteCallBack(void *contents, size_t size, size_t nmemb,
                            void *userp);

std::string http_get(const std::string &url);