#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

#include <curl/curl.h>

#include "Utils/Encoding.h"
#include "c_defs.h"
#include "Utils/sha1.h"
#include "SecretStrings.h"


#define GLOBAL_BUFF_SIZE 4096
char BUFFER[GLOBAL_BUFF_SIZE];
size_t BUFFEROffset = 0;
char* progName = NULL;
FILE *inFile = NULL;


char B2UploadSecret[50];
const char* B2apiNameVerURL = "/b2api/v2/";
const char* UsageCopyWrite = "Copyright: Tommy Lane (L&L Operations) 2020";

typedef struct 
{
	void* array;
	size_t index;
}dynamic_buffer_t;

char* calcSha1Sum(char* filepath);


char* GetUserPassword(void)
{
	char* ret = NULL;
	char buffer[1024];
	memset((void*) buffer, 0, sizeof(buffer));

	//fprintf(stdout, "Please enter a password you would trust to keep your secrets safe: ");
	//fgets(buffer, sizeof(buffer), stdin);
	ret = getpass("Please enter the password you trusted to keep your secrets safe: ");
	if (strlen(ret) == 127)
	{
		fprintf(stderr, "Hey, do you work for some three letter agency? If so use something more secure than this.\nYoumust use less then 127 characters.\n");
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%s", ret);
		fprintf(stdout, "\n");
		ret = malloc((strlen(buffer) + 1));
		if (ret != NULL)
		{
			snprintf(ret, (strlen(buffer) + 1), "%s", buffer);
		}
		else
		{
			fprintf(stderr, "Memory allocation error. Please close chrome and try again.\n");
		}
	}
	return ret;
}

void DecodeSecret(void)
{
	char c;
	char* userPassword = GetUserPassword();
	memset((void*) B2UploadSecret, 0, sizeof(B2UploadSecret));
	const char *pos = b2UploadSec1;
	for (size_t count = 0; count < sizeof(B2UploadSecret); count++) {
        sscanf(pos, "%2hhx", &B2UploadSecret[count]);
        B2UploadSecret[count] = (B2UploadSecret[count] ^ (userPassword[(count)%strlen(userPassword)]));
        pos += 2;
        if (!*pos)
        {
        	break;
        }
    }
}

void Usage(void)
{
	printf("%s\nUsage:\n\t%s <filepath for upload>\n\n", UsageCopyWrite, progName);
}

void setBuffer(uint8_t* data, size_t len)
{
	size_t outLen = 0;
	memset((void*) BUFFER, 0, sizeof(BUFFER));
	if (len >= sizeof(BUFFER))
	{
		outLen = sizeof(BUFFER) - 1;
	}
	else
	{
		outLen = len;
	}
	memcpy((void*) BUFFER, data, outLen);
}

static size_t curlFillBuffer(uint8_t* ptr, size_t size, size_t nmemb, void* stream)
{
	size_t val = 0;
	if ((size*nmemb) >= sizeof(BUFFER) - BUFFEROffset - 1)
	{
		val = sizeof(BUFFER) - BUFFEROffset - 1;
	}
	else
	{
		val = size*nmemb;
	}
	memcpy((void*) &BUFFER[BUFFEROffset], ptr, val);
	BUFFEROffset += val;
	//printf("%*s", size*nmemb, ptr);
	return size*nmemb;
}

static size_t curlFillBufferDynamic(uint8_t* ptr, size_t size, size_t nmemb, void* stream)
{
	size_t val = 0;
	void* allocStatus = NULL;
	uint8_t* u8p;
	dynamic_buffer_t* dBuffer = NULL;
	if (stream != NULL)
	{
		dBuffer = (dynamic_buffer_t*) stream;
		allocStatus = realloc(dBuffer->array, (dBuffer->index + (size * nmemb) + 1));
		if (allocStatus != NULL)
		{
			dBuffer->array = allocStatus;
			memcpy((void*) &dBuffer->array[dBuffer->index], ptr, nmemb * size);
			dBuffer->index += (nmemb * size);
			// u8p = (uint8_t*) &dBuffer->array[dBuffer->index];
			// u8p = 0;
			val = (nmemb * size);
		}
	}
	//LOG_DEBUG("returning successfully.\n");
	return val;
}

char* getDynamicStringCopy(char* ntString)
{
	char* ret = NULL;
	size_t len = strlen(ntString) + 1;
	ret = malloc(len);
	if (ret != NULL)
	{
		memset((void*) ret, 0, len);
		snprintf(ret, len, "%s",ntString);
	}
	return ret;
}

char* getValueFromResp(char* key, char* response)
{
	char* ret = NULL;
	char* cp = NULL;
	char* cp1 = NULL;
	size_t valueLen = 0;
	//LOG_DEBUG("key: %s\n", key);
	cp = strstr(response, key);
	if (cp != NULL)
	{
		// LOG_DEBUG("KEY CONTAINED IN REPONSE.\n");
		cp = strstr(cp, ":");
		if (cp != NULL)
		{
			cp = strstr(cp, "\"");
			cp++;
			if (cp != NULL)
			{
				cp1 = strstr(cp, "\"");

				if (cp1 != NULL)
				{
					// LOG_DEBUG("Value: \n\n\n\n%*s\n\n\n\n\n", cp1-cp, cp);
					valueLen = cp1 - cp + 1;
					ret = malloc(valueLen);
					if (ret != NULL)
					{
						memset((void*) ret, 0, valueLen);
						snprintf(ret, valueLen, "%*s", (int)valueLen-1, cp);
					}
				}
			}
		}
	}
	return ret;
}


dynamic_buffer_t* makeCurlGetReq(char* url, CURLcode* resp_p, char* authString)
{
	dynamic_buffer_t* ret = NULL;
	CURL* curl;
	CURLcode response = 0;
	uint8_t* u8p;
	if (url != NULL)
	{
		ret = malloc(sizeof(dynamic_buffer_t));
		if (ret != NULL)
		{
			memset((void*) ret, 0, sizeof(dynamic_buffer_t));
			ret->array = malloc(1);
			if (ret->array != NULL)
			{
				curl = curl_easy_init();
				if (curl != NULL)
				{
					curl_easy_setopt(curl, CURLOPT_URL, url);
					if (authString != NULL)
					{
						curl_easy_setopt(curl, CURLOPT_USERPWD, authString);
					}
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFillBufferDynamic);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, ret);
					response = curl_easy_perform(curl);
					if (resp_p != NULL)
					{
						*resp_p = response;
					}
					LOG_DEBUG("cleaning up.\n");
					curl_easy_cleanup(curl);
				}
				else
				{
					LOG_ERROR("curl was null");
					free(ret->array);
					free(ret);
					ret = NULL;
				}
			}
			else
			{
				free(ret);
				ret = NULL;
			}
		}
	}
	// LOG_DEBUG("ret->array: %s\n", ret->array);
	return ret;
}


dynamic_buffer_t* makeCurlPostReq(CURL** curl_p, CURLcode* resp_p, char* data, struct curl_slist* chunkList)
{
	dynamic_buffer_t* ret = NULL;
	CURL* curl;
	CURLcode response = 0;
	// LOG_DEBUG("ret->array: %s\n", ret->array);



	if ((curl_p != NULL) && (*curl_p != NULL) && (data != NULL) )
	{
		ret = malloc(sizeof(dynamic_buffer_t));
		if (ret != NULL)
		{
			memset((void*) ret, 0, sizeof(dynamic_buffer_t));
			ret->array = malloc(1);
			if (ret->array != NULL)
			{
				//curl = curl_easy_init();
				curl = *curl_p;
				if (curl != NULL)
				{
					//curl_easy_setopt(curl, CURLOPT_URL, url);
					if (chunkList != NULL)
					{
						curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunkList);
					}
					//Don't need to check null.
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFillBufferDynamic);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, ret);
					response = curl_easy_perform(curl);
					if (resp_p != NULL)
					{
						*resp_p = response;
					}
					//LOG_DEBUG("cleaning up.\n");
					//Not mine to clean.
					//curl_easy_cleanup(curl);
				}
				else
				{
					LOG_ERROR("curl was null");
					free(ret->array);
					free(ret);
					ret = NULL;
				}
			}
			else
			{
				free(ret);
				ret = NULL;
			}
		}
	}


	return ret;
}
size_t readFileCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	size_t status = 0;
	FILE* file = (FILE*) userdata;
	uint8_t* rawData[65536];

	memset((void*) rawData, 0, sizeof(rawData));
	status = (size*nitems);
	if (sizeof(rawData) > (size*nitems))
	{
		status = (size*nitems);
	}
	status = fread(rawData, 1, status, file);
	memcpy((void*) buffer, (void*) rawData, status);
	return status;
}



bool openFile(char* filepath)
{
	bool ret = false;
	//fopen(const char * restrict path, const char * restrict mode);
	inFile = fopen(filepath, "rb");
	if (inFile != NULL)
	{
		ret = true;
	}
	return ret;
}

size_t getFileSize(FILE* file)
{
	size_t ret = 0;
	struct stat st;
	int fd = fileno(file);
	fstat(fd, &st);
	ret = st.st_size;
	return ret;
}

int main(int argc, char** argv, char** envp)
{
	int ret = -1;
	CURL* curl;
	CURLcode response = 0;
	long serverResp = 0;
	curl_mime *post_mime = NULL;
  	curl_mimepart *field = NULL;
  	char* tempString = NULL;
	struct curl_slist *chunk = NULL;
	char* fullAuthString = NULL;
	char* authTok = NULL;
	char* bucketId = NULL;
	char* apiUrl = NULL;
	char* postUrlResponse = NULL;
	char* postUrl = NULL;
	char* postDataString = NULL;
	char* postAuthTok = NULL;
	char* sha1Sum = NULL;
	char* cp;
	progName = argv[0];
	if (argc < 2)
	{
		Usage();
		LOG_ERROR("Filename not available\n");
		ret = 1;
	}
	else
	{
		DecodeSecret();
		memset((void*) BUFFER, 0, sizeof(BUFFER));
		LOG_DEBUG("Upload scret: %s", B2UploadSecret);
		snprintf(BUFFER, sizeof(BUFFER), "%s:%s", B2UploadKeyId, B2UploadSecret);
		curl = curl_easy_init();
		if (curl != NULL)
		{
			dynamic_buffer_t* dArray = makeCurlGetReq("https://api.backblazeb2.com/b2api/v2/b2_authorize_account",
													&response, BUFFER);
			if (response != CURLE_OK)
			{
				LOG_ERROR("curl_easy_perform() failed: %s\n", curl_easy_strerror(response));
				ret = -1;
			}
			else
			{
				if (dArray != NULL && dArray->array != NULL && dArray->index > 0)
				fullAuthString = (char*) dArray->array;
				free(dArray);
				dArray = NULL;
				if (fullAuthString != NULL)
				{
					LOG_DEBUG("fullAuthString: %s", fullAuthString);
					authTok = getValueFromResp("authorizationToken", fullAuthString);
									
					if (authTok != NULL)
					{
						//fullAuthString valid, and authTok valid.
						LOG_DEBUG("authorizationToken: \n%s\n", authTok);
						LOG_DEBUG("fullAuthString: %s\n", fullAuthString);
						bucketId = getValueFromResp("bucketId", fullAuthString);
						if (bucketId != NULL)
						{
							LOG_DEBUG("bucketId: %s\n", bucketId);
							apiUrl = getValueFromResp("apiUrl", fullAuthString);
						
							if (apiUrl != NULL)
							{
								LOG_DEBUG("apiUrl: \n%s\n", apiUrl);
								curl_easy_setopt(curl, CURLOPT_USERPWD, NULL);
								//curl_easy_reset(curl);

								snprintf(BUFFER, sizeof(BUFFER), "%s%s%s", apiUrl, B2apiNameVerURL, "b2_get_upload_url");
								LOG_DEBUG("get upload url: %s\n", BUFFER);
								curl_easy_setopt(curl, CURLOPT_URL, BUFFER);
								memset((void*) BUFFER, 0, sizeof(BUFFER));

								chunk = curl_slist_append(chunk, "Accept:");

								snprintf(BUFFER, sizeof(BUFFER), "%s: %s", "Authorization", authTok);
								LOG_DEBUG("auth header: %s\n", BUFFER);
								chunk = curl_slist_append(chunk, BUFFER);
								//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

								snprintf(BUFFER, sizeof(BUFFER), "{\"bucketId\": \"%s\"}", bucketId);
								postDataString = getDynamicStringCopy(BUFFER);
								if (postDataString != NULL)
								{
									//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postDataString);
									memset((void*) BUFFER, 0, sizeof(BUFFER));
									LOG_DEBUG("postDataString: %s\n", postDataString);

									//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); 
									//dynamic_buffer_t* makeCurlPostReq(CURL** curl_p, CURLcode* resp_p, char* data, struct curl_slist* chunkList)
									dArray = makeCurlPostReq(&curl, &response, postDataString, chunk);
									curl_slist_free_all(chunk);
									chunk = NULL;
									if (dArray != NULL)
									{

										//response = curl_easy_perform(curl);
										if (response != CURLE_OK)
										{
											LOG_ERROR("curl_easy_perform() failed: %s\n", curl_easy_strerror(response));
										}
										else
										{
											//Got back valid response
											long responseCode;
											LOG_DEBUG("curl call was ok\n");
											curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

											LOG_DEBUG("response code; Code: %ld\n", responseCode);
											if (responseCode < 300 && responseCode > 199)
											{
												//Valid url recieved.
												postUrlResponse = dArray->array;
												free(dArray);
												if (postUrlResponse != NULL)
												{
													memset((void*) BUFFER, 0, sizeof(BUFFER));
													LOG_DEBUG("postUrlResponse: %s\n", postUrlResponse);
													postUrl = getValueFromResp("uploadUrl", postUrlResponse);
													if (postUrl != NULL)
													{
														LOG_DEBUG("postUrl: %s\n", postUrl);
														postAuthTok = getValueFromResp("authorizationToken", postUrlResponse);
													}


													free(postUrlResponse);
												}
											}
											else
											{
												LOG_ERROR("buffer after failed get upload url: %s\n", BUFFER);
											}
										}
									}
									free(postDataString);
								}

								free(apiUrl);
								apiUrl = NULL;
							}
							//Removed as reused in final upload call.
							//free(bucketId);
						}
					
						free(authTok);
						authTok = NULL;
						
					}
					free(fullAuthString);
					fullAuthString = NULL;
				}
			}
			if (postAuthTok != NULL)
			{
				//Means everything has completed and I have what I need to perform upload.

				curl_easy_reset(curl);
				curl_easy_setopt(curl, CURLOPT_URL, postUrl);
				chunk = curl_slist_append(chunk, "Accept:");
				memset((void*) BUFFER, 0, sizeof(BUFFER));
				snprintf(BUFFER, sizeof(BUFFER), "%s: %s", "Authorization", postAuthTok);
				LOG_DEBUG("post auth header: %s\n", BUFFER);
				chunk = curl_slist_append(chunk, BUFFER);
				memset((void*) BUFFER, 0, sizeof(BUFFER));
				//X-Bz-File-Name
				LOG_DEBUG("Calling urlEncode\n");
				tempString = urlEncode(argv[1], strlen(argv[1]));
				LOG_DEBUG("urlEncode returned.\n");
				if (tempString != NULL)
				{
					LOG_DEBUG("urlEncodedString: %s\n", tempString);
					snprintf(BUFFER, sizeof(BUFFER), "%s: %s", "X-Bz-File-Name", tempString);	
					free(tempString);
					tempString = NULL;
				}
				else
				{
					snprintf(BUFFER, sizeof(BUFFER), "%s: %s", "X-Bz-File-Name", argv[1]);	
				}
				
				chunk = curl_slist_append(chunk, BUFFER);
				memset((void*) BUFFER, 0, sizeof(BUFFER));

				snprintf(BUFFER, sizeof(BUFFER), "%s: %s", "Content-Type", "b2/x-auto");
				chunk = curl_slist_append(chunk, BUFFER);
				memset((void*) BUFFER, 0, sizeof(BUFFER));

				

				
				//snprintf(BUFFER, sizeof(BUFFER), "%s");
				
				if (openFile(argv[1]) == true)
				{
					//Might have checked file valid before doing all this...
					//TODO: make better. Lol.
					sha1Sum = calcSha1Sum(argv[1]);
					if (sha1Sum != NULL)
					{
						snprintf(BUFFER, sizeof(BUFFER), "%s: %s", "X-Bz-Content-Sha1", sha1Sum);
						chunk = curl_slist_append(chunk, BUFFER);
						memset((void*) BUFFER, 0, sizeof(BUFFER));

						snprintf(BUFFER, sizeof(BUFFER), "%s: %zu", "Content-Length", getFileSize(inFile));
						chunk = curl_slist_append(chunk, BUFFER);
						memset((void*) BUFFER, 0, sizeof(BUFFER));

						curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
						curl_easy_setopt(curl, CURLOPT_POST, 1L);
						curl_easy_setopt(curl, CURLOPT_READFUNCTION, readFileCallback);
						curl_easy_setopt(curl, CURLOPT_READDATA, inFile);
						curl_easy_setopt(curl, CURLINFO_RESPONSE_CODE, &serverResp);
						// post_mime = curl_mime_init(curl);
						// field = curl_mime_addpart(post_mime);
					    // curl_mime_name(field, "sendfile");
					    // curl_mime_filedata(field, argv[1]);
					    // curl_easy_setopt(curl, CURLOPT_MIMEPOST, post_mime);
					    memset((void*) BUFFER, 0, sizeof(BUFFER));
					    BUFFEROffset = 0;
					    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFillBuffer);
						response = curl_easy_perform(curl);
						LOG_DEBUG("Upload Response: %s\n", BUFFER);
						curl_slist_free_all(chunk);
						chunk = NULL;
						free(sha1Sum);
						LOG_DEBUG("reponse code: %ld", serverResp);
						if (response == CURLE_OK && serverResp == 0)
						{
							ret = 0;
						}
					}
				}
				else
				{
					LOG_ERROR("\n\nFile: \"%s\"\tWas not found.\n\n", argv[1]);
					ret = 2;
				}



				free(postAuthTok);
				postAuthTok = NULL;
				free(bucketId);
				bucketId = NULL;
			}
			else
			{
				LOG_ERROR("User key: %s, not authorized for upload on bucket: %s\n", B2UploadKeyId, INJEST_BUCKET);
				ret = 3;
			}
			if (postUrl != NULL)
			{
				free(postUrl);
				postUrl = NULL;
			}

			LOG_DEBUG("cleaning up.\n");
			curl_easy_cleanup(curl);
		}
	}
	/* code */
	return ret;
}