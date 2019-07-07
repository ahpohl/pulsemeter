// c++ headers
#include <iostream>
#include <iomanip>
#include <ctime>

// c headers
#include <curl/curl.h>

extern "C" {
#include <rrd.h>
#include <rrd_client.h>
}

// program headers
#include "pulse.hpp"

using namespace std;

// set PVOutput.org parameters
void Pulse::setPVOutput(char const* t_apikey, char const* t_sysid, 
  char const* t_url)
{
  if ((!t_apikey || !t_sysid || !t_apikey))
  {
    throw runtime_error("Please set PVOutput.org api key, system id and add status url");
  }

  m_apikey = t_apikey;
  m_sysid = t_sysid;
  m_url = t_url;
}

size_t Pulse::curlCallback(void * t_contents, size_t t_size, 
  size_t t_nmemb, void * t_user)
{
  ((string*)t_user) -> append((char*)t_contents, (t_size * t_nmemb));
    
  return (t_size * t_nmemb);
}

// upload energy and power to PVOutput.org
void Pulse::uploadToPVOutput(void) const
{
	CURLcode res;
	string url = "https://pvoutput.org/service/r2/addstatus.jsp";
	struct curl_slist * headers = nullptr;
	string api_key_header = string("X-Pvoutput-Apikey: ") + m_apikey;
	string sys_id_header = string("X-Pvoutput-SystemId: ") + m_sysid;
	string read_buffer;
  char date_buffer[12] = {0};
  char time_buffer[12] = {0};

  // check if time has been set
  if (m_time == 0)
  {
    throw runtime_error("setTime(): timestamp not set");
  }

	// create curl easyhandle
	CURL * easyhandle = curl_easy_init();

	// CURL verbose
	if (m_debug)
	{
		curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1L);
	}

	// append headers
	headers = curl_slist_append(headers, api_key_header.c_str());
	headers = curl_slist_append(headers, sys_id_header.c_str());

	// pass list of custom headers
	curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, headers);

	// format date and time
	struct tm * tm = localtime(&m_time);
  strftime(date_buffer, 11, "%Y%m%d", tm);
	strftime(time_buffer, 11, "%R", tm);

	// append data
	string data = string("d=") + date_buffer
		+ "&t=" + time_buffer 
		+ "&v3=" + to_string(m_energy)
		+ "&v4=" + to_string(m_power)
		+ "&c1=1";

	// pass list to curl
	curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, data.c_str());

	// pass url to curl
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());

	// catch the answer
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, curlCallback);
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &read_buffer);

	// transfer http
	res = curl_easy_perform(easyhandle);
	
	// error handling
	if (res != CURLE_OK)
	{
		throw runtime_error(string("curl_easy_perform() failed: ") 
			+ curl_easy_strerror(res));
	}
 
	// cleanup curl handle
	curl_easy_cleanup(easyhandle);

	// free the custom headers
  curl_slist_free_all(headers);

	// output the CURL answer on screen	
	cout << "PVOutput response: " << read_buffer << endl;
}
