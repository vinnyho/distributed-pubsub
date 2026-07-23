
#include "request.h"
// void* pointer to unknow ntype
static size_t WriteCallBack(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

std::string http_get(const std::string &url) {
  CURL *curl;
  CURLcode res = CURLE_FAILED_INIT;
  std::string response;

  curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  if (res != CURLE_OK) {
    std::cerr << "Failed" << std::endl;
  }
  return response;
}