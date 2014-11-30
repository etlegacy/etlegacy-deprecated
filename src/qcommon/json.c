/**
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 *
 * @file json.c
 */

#include <curl/curl.h>

#define JSON_API_MEM_BURST (16 * 1024)

#include "q_shared.h"
#include "qcommon.h"
#include "dl_public.h"

typedef struct json_api_req_s
{
	json_api_callback callback;
	void *user;

	char *request;
	char *buf;
	size_t bufSize;
	size_t bufPos;
} json_api_req_t;

static CURLM *curl_multi;

static int json_api_write_func(char *ptr, size_t size, size_t nmemb, json_api_req_t *request)
{
	size_t total = (size * nmemb);

	if (request->buf == NULL) {
		request->bufSize = JSON_API_MEM_BURST > total ? JSON_API_MEM_BURST : total;
		request->buf = malloc(request->bufSize);
		request->bufPos = 0;
	}

	if (request->bufPos + size * nmemb > request->bufSize) {
		request->bufSize += JSON_API_MEM_BURST > total ? JSON_API_MEM_BURST : total;
		request->buf = realloc(request->buf, request->bufSize);
	}

	if (!request->buf)
		return 0;

	memcpy(request->buf + request->bufPos, ptr, size * nmemb);
	request->bufPos += size * nmemb;

	return size * nmemb;
}

void json_api_init()
{
	if (curl_multi)
		return;

	if (curl_global_init(CURL_GLOBAL_NOTHING) != CURLE_OK) {
		Com_Error(ERR_FATAL, "curl_global_init failed.");
		return;
	}

	curl_multi = curl_multi_init();
	if (!curl_multi) {
		Com_Error(ERR_FATAL, "curl_multi_init failed.");
		return;
	}
}

int json_api_request(const char *url, json_t *payload, json_api_callback callback, void *user)
{
	json_api_req_t *req = NULL;
	CURL *curl = NULL;

	if (!curl_multi)
		goto error;

	req = calloc(sizeof(json_api_req_t), 1);
	if (!req)
		goto error;

	req->callback = callback;
	req->user = user;

	if (payload) {
		req->request = json_dumps(payload, 0);
		if (!req->request)
			goto error;
	}

	curl = curl_easy_init();
	if (!curl)
		goto error;

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_INTERFACE, Cvar_VariableString("net_ip"));
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, json_api_write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);
	if (req->request) {
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->request);
	} else {
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
	}
	curl_easy_setopt(curl, CURLOPT_PRIVATE, req);

	if (curl_multi_add_handle(curl_multi, curl) != CURLM_OK)
		goto error;

	Com_DPrintf("json_api_request(url=\"%s\", payload=%p, callback=%p, user=%p)\n", url, payload, callback, user);

	return 1;

error:
	if (curl)
		free(curl);

	if (req && req->request)
		free(req->request);

	if (req)
		free(req);

	return 0;
}

static void json_api_request_free(json_api_req_t *req)
{
	if (req->request)
		free(req->request);

	if (req->buf)
		free(req->buf);

	free(req);
}

int json_api_frame()
{
	CURLMsg *curl_msg;
	int handles = 0;
	int ret = 0;

	if (!curl_multi || curl_multi_perform(curl_multi, &handles) != CURLM_OK)
		return 0;

	ret = handles;

	while ( (curl_msg = curl_multi_info_read(curl_multi, &handles)) ) {
		CURL *curl = curl_msg->easy_handle;
		json_api_req_t *request = NULL;
		long code = 0;

		if (curl_msg->msg != CURLMSG_DONE)
			continue;

		curl_easy_getinfo(curl, CURLINFO_PRIVATE, &request);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

		if (request && request->callback) {
			json_t *payload = NULL;
			int success = (curl_msg->data.result == CURLE_OK && code == 200);

			if (request->buf)
				payload = json_loadb(request->buf, request->bufPos, 0, NULL);

			Com_DPrintf("json_api_pump: %p->callback(success=%s, payload=\"%s\", user=%p)\n", request->callback, success ? "TRUE" : "FALSE", json_dumps(payload, 0), request->user);

			request->callback(success, payload, request->user);

			if (payload)
				json_decref(payload);
		}

		json_api_request_free(request);

		curl_multi_remove_handle(curl_multi, curl);
		curl_easy_cleanup(curl);
	} 

	return ret;
}

void json_api_free()
{
	if (!curl_multi)
		return;

	curl_multi_cleanup(curl_multi);
	curl_global_cleanup();
	curl_multi = NULL;
}
