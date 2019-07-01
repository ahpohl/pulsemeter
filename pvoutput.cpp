#include <iostream>
#include <ctime>
#include <iomanip>
#include <vector>

extern "C" {
#include <rrd.h>
#include <rrd_client.h>
#include <curl/curl.h>
}

#include "pulse.h"

using namespace std;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, 
	void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


// upload energy and power to PVOutput.org
void Pulse::RRDUploadToPVOutput(time_t end_time)
{
	CURLcode res;
	string url = "https://pvoutput.org/service/r2/addstatus.jsp";
	struct curl_slist * headers = nullptr;
	string api_key_header = string("X-Pvoutput-Apikey: ") + PVOutputApiKey;
	string sys_id_header = string("X-Pvoutput-SystemId: ") + PVOutputSysId;
	string readBuffer;

	// create curl easyhandle
	CURL * easyhandle = curl_easy_init();

	// CURL verbose
	if (Debug)
	{
		curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1L);
	}

	// append headers
	headers = curl_slist_append(headers, api_key_header.c_str());
	headers = curl_slist_append(headers, sys_id_header.c_str());

	// pass list of custom headers
	curl_easy_setopt(easyhandle, CURLOPT_HTTPHEADER, headers);

	// append data
	string data = "d=" + to_string(end_time) 
		+ "&v3=" + to_string(Energy)
		+ "&v4=" + to_string(Power)
		+ "&c1=1";

	// pass list to curl
	curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, data.c_str());

	// pass url to curl
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());

	// catch the answer
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &readBuffer);

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
	cout << "PVOutput response: " << readBuffer << endl;
}

