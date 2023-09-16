#include "upload.h"

#include "configuration.h"

#include <assert.h>
#include <map>
#include <string>

#include <curl/curl.h>

namespace breakpad
{

static size_t on_response_data(void *ptr, size_t size, size_t nmemb, void *userp)
{
  if (!userp)
    return 0;

  std::string *response = reinterpret_cast<std::string *>(userp);
  size_t real_size = size * nmemb;
  response->append(reinterpret_cast<char *>(ptr), real_size);
  return real_size;
}


bool upload(const Configuration &cfg, const std::string &url, std::string &response)
{
  char error_buffer[CURL_ERROR_SIZE] = {0};
  CURL *curl = curl_easy_init();
  assert(curl);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, cfg.userAgent.c_str());

  struct curl_httppost *form = NULL;
  struct curl_httppost *last = NULL;

  typedef std::map<std::string, std::string>::const_iterator map_iter;
  for (map_iter it = cfg.params.begin(); it != cfg.params.end(); ++it)
    curl_formadd(&form, &last, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, it->second.c_str(), CURLFORM_END);

  for (map_iter it = cfg.files.begin(); it != cfg.files.end(); ++it)
    curl_formadd(&form, &last, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_FILE, it->second.c_str(), CURLFORM_END);

  curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);
  struct curl_slist *headerlist = NULL;
  headerlist = curl_slist_append(headerlist, "Expect:");
  headerlist = curl_slist_append(headerlist, "X-Breakpad-Epoch: 2");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_response_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&response));

  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
  CURLcode err_code = curl_easy_perform(curl);
  if (err_code != CURLE_OK)
  {
    response = curl_easy_strerror(err_code);
    response += "\n";
    response += error_buffer;
  }

  if (headerlist)
    curl_slist_free_all(headerlist);

  if (form)
    curl_formfree(form);

  curl_easy_cleanup(curl);

  return err_code == CURLE_OK;
}

} // namespace breakpad
