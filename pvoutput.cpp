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

// upload energy and power to PVOutput.org
void Pulse::RRDUploadToPVOutput(time_t end_time)
{
  	string url = "https://pvoutput.org/service/r2/addstatus.jsp";
	struct curl_slist * headers = nullptr;
	string api_key_header = string("X-Pvoutput-Apikey: ") + PVOutputApiKey;
	string sys_id_header = string("X-Pvoutput-SystemId: ") + PVOutputSysId;

	// create curl easyhandle
	CURL * easyhandle = curl_easy_init();

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

	if (Debug)
	{
		cout << "curl POST data: " << data << endl;
	}

	// pass list to curl
	curl_easy_setopt(easyhandle, CURLOPT_POSTFIELDS, data.c_str());

	// pass url to curl
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());

	// transfer http
	curl_easy_perform(easyhandle);
 
	// free the header list
	curl_slist_free_all(headers);

	// cleanup curl handle
	curl_easy_cleanup(easyhandle);
}

