#include <stdio.h>
#include <curl/curl.h>
#include "fileSend.h"


// https://curl.se/libcurl/c/http-post.html

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

