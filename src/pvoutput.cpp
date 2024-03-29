#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <curl/curl.h>
#include "pulse.hpp"

using namespace std;

// set PVOutput.org parameters
void Pulse::setPVOutput(char const* t_apikey, char const* t_sysid, 
  char const* t_url, int const& t_interval)
{
  if ((!t_apikey) || (!t_sysid) || (!t_url) || (!t_interval)) {
    cout << "Upload to PVOutput disabled" << endl;
    return;
  } 
  
  if (!(t_interval == 5 || t_interval == 10 || t_interval == 15)) {
    throw runtime_error(string("Upload to PVOutput every ")
      + to_string(t_interval) + " minutes not supported");
  }
  
  m_pvoutput = true;
  m_apikey = t_apikey;
  m_sysid = t_sysid;
  m_url = t_url;
  m_interval = t_interval;
    
  if (m_debug) {
    cout << "Upload to PVOutput every " << t_interval
      << " minutes" << endl;
  }
}

size_t Pulse::curlCallback(void * t_contents, size_t t_size, 
  size_t t_nmemb, void * t_user)
{
  ((string*)t_user) -> append((char*)t_contents, (t_size * t_nmemb));
    
  return (t_size * t_nmemb);
}

void Pulse::uploadXport(void) const
{
  if (!m_pvoutput) {
    return;
  }

  time_t endtime = 0;
  double energy = 0;
  double power = 0;
  time_t rawtime = time(nullptr);
 
  getEnergyAndPower(rawtime, (m_interval * 60), &endtime, &energy, &power);
  struct tm* tm = localtime(&endtime);
 
  char date_buffer[12] = {0};
  char time_buffer[12] = {0};
  strftime(date_buffer, 11, "%Y%m%d", tm);
  strftime(time_buffer, 11, "%R", tm);

  string data = string("d=") + date_buffer
    + "&t=" + time_buffer
    + "&v3=" + to_string(energy)
    + "&v4=" + to_string(power)
    + "&c1=1";

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

  curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(easyhandle, CURLOPT_URL, m_url);
  curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, curlCallback);

  string read_buffer;
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &read_buffer);

  CURLcode res = curl_easy_perform(easyhandle);
  curl_easy_cleanup(easyhandle);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    cout << "Upload to PVOutput failed: " 
         << curl_easy_strerror(res) << endl;
    return;
  }
 
  char buffer[32] = {0};
  strftime(buffer, 31, "%F %T", tm);

  cout << "Date: " << buffer
    << ", energy: " << fixed << setprecision(1) << energy / 1000.0
    << " kWh, power: " << setprecision(1) << power << " W, sys id: "
    << m_sysid << endl;

  cout << "PVOutput response: " << read_buffer << endl;

  if (m_debug) {
    ofstream log;
    log.open("xport.log", ios::app);

    // Date,Timestamp,Energy [Wh],Power [W]
    log << buffer << "," << endtime << "," << fixed << setprecision(1)
      << energy << "," << power << endl;

    log.close();
  }
}
