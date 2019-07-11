// c++ headers
#include <iostream>
#include <fstream>
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
  if ( (!t_apikey) || (!t_sysid) || (!t_url) ) {
    cout << "Upload to PVOutput.org disabled" << endl;
  } else {
    m_pvoutput = true;
    m_apikey = t_apikey;
    m_sysid = t_sysid;
    m_url = t_url;
  }
}

size_t Pulse::curlCallback(void * t_contents, size_t t_size, 
  size_t t_nmemb, void * t_user)
{
  ((string*)t_user) -> append((char*)t_contents, (t_size * t_nmemb));
    
  return (t_size * t_nmemb);
}

void Pulse::uploadToPVOutput(void) const
{
  if (!m_pvoutput) {
    return;
  }

  int const STEPS = 12;
  int interval[STEPS] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
  int *p = interval;
  time_t rawtime = time(nullptr);
  struct tm* tm = localtime(&rawtime);
  bool is_time = false;

  for (int i = 0; i < STEPS; ++i, ++p) {
    if ((*p == tm->tm_min) && (tm->tm_sec == 0)) {
      is_time = true;
      break;
    }
  }

  if (!is_time) {
    return;
  }

	CURL * easyhandle = curl_easy_init();

	if (m_debug) {
		curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1L);
	}

  struct curl_slist * headers = nullptr;
  string api_key_header = string("X-Pvoutput-Apikey: ") + m_apikey;
  string sys_id_header = string("X-Pvoutput-SystemId: ") + m_sysid;
	
  headers = curl_slist_append(headers, api_key_header.c_str());
	headers = curl_slist_append(headers, sys_id_header.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, headers);

  double energy = 0;
  double power = 0;
 
  getEnergyAndPower(rawtime, &energy, &power);

  char date_buffer[12] = {0};
  char time_buffer[12] = {0};
  
  strftime(date_buffer, 11, "%Y%m%d", tm);
	strftime(time_buffer, 11, "%R", tm);

	string data = string("d=") + date_buffer
		+ "&t=" + time_buffer 
		+ "&v3=" + to_string(energy)
		+ "&v4=" + to_string(power)
		+ "&c1=1";

	curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, data.c_str());
	curl_easy_setopt(easyhandle, CURLOPT_URL, m_url);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, curlCallback);
  
  string read_buffer;
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &read_buffer);

	CURLcode res = curl_easy_perform(easyhandle);
	if (res != CURLE_OK) {
		throw runtime_error(string("Curl_easy_perform() failed: ") 
			+ curl_easy_strerror(res));
	}
 
	curl_easy_cleanup(easyhandle);
  curl_slist_free_all(headers);
	
  cout << "PVOutput response: " << read_buffer << endl;

  bool is_log = true;
  if (is_log) {
    static unsigned long previous_counter = 0;
    double my_energy = static_cast<double>(m_counter) / m_rev * 1000;
    double my_power = (static_cast<double>(m_counter) - previous_counter) / 
      m_rev * 1000 * 3600 / 300;
    previous_counter = m_counter;

    ofstream log;
    log.open("pulse.log", ios::app);
    

    log << date_buffer << " " << time_buffer << "," << rawtime << "," 
        << m_counter << "," << fixed << setprecision(1) 
        << my_energy << "," << energy << "," 
        << my_power << "," << power << endl;
    
    log.close();
  }
}
