#include <stdio.h>
#include <curl/curl.h>
#include "fileSend.h"


// https://curl.se/libcurl/c/http-post.html
int sendRealTime(const char *url, const char *data) {
	CURL *curl;
	CURLcode res;
	char buff[512] = {0, }; 
	snprintf(buff, sizeof(buff), "data=%s", data);

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if(curl) {
		/* First set the URL that is about to receive our POST. This URL can
		 *        just as well be a https:// URL if that is what should receive the
		 *               data. */
		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* Now specify the POST data */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buff);
		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK){
			return -1;
			//	fprintf(stderr, "curl_easy_perform() failed: %s\n",
			//			curl_easy_strerror(res));
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return 0;
}


int sendFile(const char *url, const char *fileName) {
	CURL * curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	curl_mime *form = NULL;
	curl_mimepart *field = NULL;
	
	if (curl) {
		/* Create the form */
		form = curl_mime_init(curl);

		/* Fill in the file upload field */
		field = curl_mime_addpart(form);
		curl_mime_name(field, "file");
		curl_mime_filedata(field, fileName);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

		res = curl_easy_perform(curl);

		/* Check for errors */
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the form */
		curl_mime_free(form);
		/* free slist */
		
	}
	curl_global_cleanup();
	return 0;
	
}

