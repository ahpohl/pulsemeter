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
#include "rrd.hpp"

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

void Pulse::logAverage(void) const
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

  static time_t previous_time = 0;
  unsigned long counter = getEnergyCounter();
  static unsigned long previous_counter = 0;

  double energy = 1000.0 * counter / m_rev;
  double power = 0;
  if (previous_time) {
    power = 3600.0 * 1000.0 * (counter - previous_counter) /
      (m_rev * (rawtime - previous_time));
  }
  previous_time = rawtime;
  previous_counter = counter;

  char buffer[32] = {0};
  strftime(buffer, 31, "%F %T", tm);

  ofstream log;
  log.open("average.log", ios::app);

  // Date,Timestamp,Counter,Energy [Wh],Power [W]
  log << buffer << "," << rawtime << "," << counter << "," 
    << fixed << setprecision(1) << energy << "," << power << endl;

  log.close();
}

void Pulse::uploadXport(void) const
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

  // upload at interval[steps] plus the offset in minutes
  // because the rrd average consolitation won't be ready at
  // exact intervals, so upload will happen later.
  // PVOutput.org will correct the offsets and saves the 
  // data at exact intervals
  for (int i = 0; i < STEPS; ++i, ++p) {
    if (((*p + Con::RRD_MIN_OFFSET) == tm->tm_min) && (tm->tm_sec == 0)) {
      is_time = true;
      break;
    }
  }

  if (!is_time) {
    return;
  }

  time_t endtime = 0;
  double energy = 0;
  double power = 0;
 
  getEnergyAndPower(rawtime, &endtime, &energy, &power);
  tm = localtime(&endtime);
  
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
  if (res != CURLE_OK) {
    throw runtime_error(string("Curl_easy_perform() failed: ")
      + curl_easy_strerror(res));
  }
 
  curl_easy_cleanup(easyhandle);
  curl_slist_free_all(headers);

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
