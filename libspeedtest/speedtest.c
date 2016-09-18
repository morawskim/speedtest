#include <asm-generic/errno-base.h>
#include "speedtest.h"

SpeedTestResult speed_test_request_get_servers_list() {
    struct MemoryChunk *chunk = (MemoryChunk *)malloc(sizeof(MemoryChunk));
    if (chunk == NULL) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    chunk->memory = (char*)malloc(900000);  /* will be grown as needed by the realloc above */
    chunk->size = 0;    /* no data at this point */
    chunk->allocated = 900000;    /* no data at this point */
    if (chunk->memory == NULL) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        speed_test_free_memory_chunk(chunk);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_CURL_INIT);
    }

    curl_easy_setopt(curl, CURLOPT_URL, "http://c.speedtest.net/speedtest-servers-static.php");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest/0.1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);


    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return speed_test_create_result_curl_error(res);
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
    return speed_test_create_result(chunk);
}

SpeedTestResult speed_test_parse_server_node(xmlDocPtr doc, xmlNodePtr cur) {
    SpeedTestServer *ret = (SpeedTestServer *) malloc(sizeof(SpeedTestServer));
    if (ret == NULL) {
        return speed_test_create_result_posix_error(ENOMEM);
    }
    memset(ret, 0, sizeof(SpeedTestServer));


    if ((!xmlStrcmp(cur->name, (const xmlChar *) "server"))) {
        ret->url = xmlGetProp(cur, (xmlChar*)"url");
        char *lat = (char*)xmlGetProp(cur, (xmlChar*)"lat");
        char *lon = (char*)xmlGetProp(cur, (xmlChar*)"lon");
        ret->lat = atof(lat);
        ret->lon = atof(lon);
        ret->name = xmlGetProp(cur, (xmlChar*)"name");
        ret->country = xmlGetProp(cur, (xmlChar*)"country");
        ret->cc = xmlGetProp(cur, (xmlChar*)"cc");
        ret->sponsor = xmlGetProp(cur, (xmlChar*)"sponsor");

        char* id = (char*)xmlGetProp(cur, (xmlChar*)"id");
        ret->id = atoi(id);
        ret->url2 = xmlGetProp(cur, (xmlChar*)"url2");
        ret->host = xmlGetProp(cur, (xmlChar*)"host");

        free(id);
        free(lat);
        free(lon);

        return speed_test_create_result(ret);
    } else {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_INVALID_ELEMENT);
    }
}

int speed_test_best_server_compare(const void *a, const void *b) {
    const SpeedTestServer *s1 = *(SpeedTestServer **)a;
    const SpeedTestServer *s2 = *(SpeedTestServer **)b;

    if (s1->distance > s2->distance) {
        return 1;
    } else if (s1->distance == s2->distance) {
        return 0;
    } else {
        return -1;
    }
}

SpeedTestResult speed_test_parse_servers_list(MemoryChunk *xml)
{
    xmlDocPtr doc = xmlReadMemory(xml->memory, (int)xml->size, NULL, NULL, NULL);
    if(NULL == doc) {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_READ);
    }
    xmlNodePtr  root = xmlDocGetRootElement(doc);
    if (NULL == root) {
        xmlFreeDoc(doc);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_EMPTY_DOC);
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "settings")) {
        xmlFreeDoc(doc);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_INVALID_FILE);
    }

    /*
     * Allocate the structure to be returned.
     */
    SpeedTestServersList *ret = (SpeedTestServersList *) malloc(sizeof(SpeedTestServersList));
    if (ret == NULL) {
        xmlFreeDoc(doc);
        return speed_test_create_result_posix_error(ENOMEM);
    }
    memset(ret, 0, sizeof(SpeedTestServersList));

    /*
     * Now, walk the tree.
     */
    /* First level we expect just Jobs */
    root = root->xmlChildrenNode;
    while (root && xmlIsBlankNode(root)) {
        root = root -> next;
    }
    if ( root == 0 ) {
        xmlFreeDoc(doc);
        speed_test_free_server_link_list(ret);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_NO_CHILD);
    }

    if ((xmlStrcmp(root->name, (const xmlChar *) "servers"))) {
        xmlFreeDoc(doc);
        speed_test_free_server_link_list(ret);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_NOT_FOUND_ELEMENT);
    }

    /* Second level is a list of Job, but be laxist */
    root = root->xmlChildrenNode;

//    SpeedTestServer* tmp = (SpeedTestServer*)malloc(sizeof(SpeedTestServer*));
    SpeedTestServer *tmp;
    SpeedTestServersList *prev = ret;

    while (root != NULL) {
        if ((!xmlStrcmp(root->name, (const xmlChar *) "server"))) {
            SpeedTestResult speedTestResult = speed_test_parse_server_node(doc, root);
            if (speed_test_has_error(&speedTestResult)) {
                xmlFreeDoc(doc);
                speed_test_free_server_link_list(ret);
                return speedTestResult;
            } else {
                //musimy utworzyc nowy linklist
                tmp = speedTestResult.data;
                SpeedTestServersList * c = (SpeedTestServersList *)malloc(sizeof(SpeedTestServersList));
                c->s = tmp;
                c->next = NULL;
                prev->next = c;
                prev = c;

            }
        }
        root = root->next;
    }

    xmlFreeDoc(doc);
    return speed_test_create_result(ret);
}

char *speed_test_url_image(char *uploadUrl, int imgSize) {
    char *tmp = strdup(uploadUrl);
    char *url = dirname(tmp);

    size_t strLen = strlen(url)+30;
    char *adres = (char*)malloc(strLen);
    char *bufor = (char*)malloc(strLen);

    if (NULL == adres || NULL == bufor) {
        //no memory
        return NULL;
    }

    memset(adres, 0, strLen);
    memset(bufor, 0, strLen);
    char *prefix = "/random%dx%d.jpg";

    strncat(adres, url, strLen-30);
    strncat(adres, prefix, 25);
    snprintf(bufor, 150, adres, imgSize, imgSize);

    free(tmp);
    free(adres);

    return bufor;
}

char *speed_test_url_latency(char *uploadUrl) {
    char *tmp = strdup(uploadUrl);
    if (NULL == tmp) {
        return NULL;
    }

    char *url = dirname(tmp);

    size_t strLen = strlen(url)+30;
    char *adres = (char*)malloc(strLen);
    if (NULL == adres) {
        return NULL;
    }
    memset(adres, 0, strLen);
    char *prefix = "/latency.txt";

    strncat(adres, url, strLen-30);
    strncat(adres, prefix, 15);

    free(url);

    return adres;
}


SpeedTestResult speed_test_download_image(char *imgUrl) {
    CURL *curl = curl_easy_init();
    CURLcode res;

    if (!curl) {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_CURL_INIT);
    }


    curl_easy_setopt(curl, CURLOPT_URL, imgUrl);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest/0.1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteNuLLCallback);
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return speed_test_create_result_curl_error(res);
    }

    double val = 0;
    long code = 0;
    SpeedTestCurlStatics *statics = (SpeedTestCurlStatics *)malloc(sizeof(SpeedTestCurlStatics));
    if (NULL == statics) {
        curl_easy_cleanup(curl);
        return speed_test_create_result_posix_error(ENOMEM);
    }

    //todo mmo a co jesli zworci blad proba pobrania jakis informacji??

    /* check for bytes downloaded */
    res = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &val);
    if((CURLE_OK == res) && (val>0)) {
        statics->bytesDownload = val;
    }

    /* check for total download time */
    res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &val);
    if((CURLE_OK == res) && (val>0)) {
        statics->time = val;
    }

    /* check for average download speed */
    res = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &val);
    if((CURLE_OK == res) && (val>0)) {
        statics->speed = val*8;
    }

    /* check for average download speed */
    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if((CURLE_OK == res) && (code>0)) {
        statics->code = code;
    }

    /* always cleanup */
    curl_easy_cleanup(curl);

    return speed_test_create_result(statics);
}

void speed_test_print_statics(SpeedTestCurlStatics *statics) {
    printf("Data downloaded: %0.0f bytes.\n", statics->bytesDownload);
    printf("Total download time: %0.3f sec.\n", statics->time);
    printf("Average download speed: %0.3f kbyte/sec.\n", statics->speed / 1024);
    printf("Response code: %ld \n", statics->code );
}

SpeedTestResult speed_test_get_best_servers(SpeedTestServersList *list, const size_t max) {
    SpeedTestServer *array[max];
    for (int i=0; i<max; i++) {
        array[i] = NULL;
    }
    unsigned int j=0;

    while (list->next != NULL) {
        if (list->s != NULL) {
            if (j<max) {
                array[j] = list->s;
            } else {
                for (int i=0; i<max; i++) {
                    if (array[i] != NULL) {
                        if (list->s->distance < array[i]->distance) {
                            int last = max-1;
                            for (int x=last; x>i; x--) {
                                array[x] = array[x-1];
                            }
                            array[i] = list->s;
                            break;
                        }
                    }
                }
            }
            j++;
            if (j==max) {
                qsort(array, max, sizeof(SpeedTestServer *), speed_test_best_server_compare);
            }
            if (j>max) {
                j=max;
            }
        }
        list = list->next;
    }

    SpeedTestServersList *serverLinkList1 = (SpeedTestServersList *)malloc(sizeof(SpeedTestServersList));
    if (NULL == serverLinkList1) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    SpeedTestResult speedTestResult = speed_test_copy_server(array[0]);
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    serverLinkList1->s = speedTestResult.data;
    serverLinkList1->next = NULL;

    SpeedTestServersList *head = serverLinkList1;
    SpeedTestServersList *prev = serverLinkList1;

    for (int i=1; i<max; i++) {
        SpeedTestServersList *serverLinkList2 = (SpeedTestServersList *)malloc(sizeof(SpeedTestServersList));
        if (NULL == serverLinkList2) {
            speed_test_free_server_link_list(head);
            speed_test_create_result_posix_error(ENOMEM);
        }

        speedTestResult = speed_test_copy_server(array[i]);
        if (speed_test_has_error(&speedTestResult)) {
            speed_test_free_server_link_list(head);
            speed_test_create_result_posix_error(ENOMEM);
        }
        serverLinkList2->s = speedTestResult.data;
        serverLinkList2->next = NULL;
        prev->next = serverLinkList2;
        prev = serverLinkList2;
    }


    //powinismy tu zwracac liste SpeedTestServersList!
    return speed_test_create_result(head);
}

SpeedTestResult  speed_test_upload_file(char *url, char *binaryString, long size) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_CURL_INIT);
    }

    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, binaryString);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteNuLLCallback);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return speed_test_create_result_curl_error(res);
    }

    //statystyki
    double val = 0;
    long code = 0;
    SpeedTestCurlStatics *statics = (SpeedTestCurlStatics *)malloc(sizeof(SpeedTestCurlStatics));

    if (NULL == statics) {
        curl_easy_cleanup(curl);
        return speed_test_create_result_posix_error(ENOMEM);
    }

    /* check for bytes downloaded */
    res = curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &val);
    if((CURLE_OK == res) && (val>0)) {
        statics->bytesDownload = val;
    }

    /* check for total download time */
    res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &val);
    if((CURLE_OK == res) && (val>0)) {
        statics->time = val;
    }

    /* check for average download speed */
    res = curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &val);
    if((CURLE_OK == res) && (val>0)) {
        statics->speed = val*8;
    }

    /* check for response code*/
    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if((CURLE_OK == res) && (code>0)) {
        statics->code = code;
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
    return speed_test_create_result(statics);
}

SpeedTestResult speed_test_get_latency(char *url) {
    CURL *curl = curl_easy_init();
    CURLcode res;

    if (!curl) {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_CURL_INIT);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest/0.1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteNuLLCallback);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return speed_test_create_result_curl_error(res);
    }

    double val = 0;
    double time = 0;
    long code = 0;
    long code2 = 0;
    res = curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &val);
    if((CURLE_OK == res) && (val>0)) {
        time = val;
    }

    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code2);
    if((CURLE_OK == res) && (code2>0)) {
        code = code2;
    }
    curl_easy_cleanup(curl);

    if (code == 200) {
        SpeedTestCurlStatics* statics = (SpeedTestCurlStatics*)malloc(sizeof(SpeedTestCurlStatics));
        if (NULL == statics) {
            return speed_test_create_result_posix_error(ENOMEM);
        }

        statics->time = time;
        return speed_test_create_result(statics);
    } else {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_RESPONSE_CODE);
    }
}

SpeedTestServer *speed_test_create_empty_server_struct() {
    SpeedTestServer * srv = (SpeedTestServer *)malloc(sizeof(SpeedTestServer));
    if (srv == NULL) {
        fprintf(stderr, "Error no memory to create empty Server struct");
        exit(1);
    }

    memset(srv, 0, sizeof(SpeedTestServer));

    srv->url = NULL;
    srv->url2 = NULL;
    srv->id = 0;
    srv->sponsor = NULL;
    srv->name = NULL;

    return srv;
}

int speed_test_is_empty_server_struct(SpeedTestServer *srv) {
    return (srv == NULL) || (srv->sponsor == NULL && srv->name == NULL && srv->url == NULL && srv->url2 == NULL && srv->id ==0);
}




SpeedTestResult speed_test_get_best_server(SpeedTestServersList *list) {
    double bestLatency = 9999;
    SpeedTestServer *bestServer = NULL;


//    todo mmo dokonczyc ta metode na bazie pinga
    SpeedTestResult speedTestResult = speed_test_copy_server(list->s);
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    bestServer = speedTestResult.data;
    return speed_test_create_result(bestServer);

//    SpeedTestServersList *filtered = speed_test_get_best_servers(list, 5);
//    return filtered->s;

    //todo mmo tu powinismy wysylac pingi w celu spr ktory z nalblizszych serwerow jest najlepszy
//    while (filtered->next != NULL) {
//        if (list->s != NULL) {
//            if (list->s->latency < bestLatency && list->s->latency > 0) {
////                bestLatency = latency;
//                bestLatency = list->s->latency;
//                bestServer = list->s;
//            }
//        }
//        list = list->next;
//    }
//
//    return bestServer;
}

void speed_test_free_server_link_list(SpeedTestServersList *list) {
    while (list != NULL) {
        SpeedTestServersList *current = list;
        if (current->s != NULL) {
            speed_test_free_server(current->s);
        }
        list = current->next;
        free(current);
    }
}

SpeedTestResult speed_test_request_get_config() {
    size_t max = 8192;
    struct MemoryChunk *chunk = (MemoryChunk *)malloc(sizeof(MemoryChunk));
    chunk->memory = (char*)malloc(max);  /* will be grown as needed by the realloc above */
    chunk->size = 0;    /* no data at this point */
    chunk->allocated = max;    /* no data at this point */

    if (chunk->memory == NULL) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    CURL *curl = curl_easy_init();
    CURLcode res;

    if (!curl) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    char *url ="http://www.speedtest.net/speedtest-config.php";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest/0.1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        return speed_test_create_result_curl_error(res);
    }

    int code = 0;
    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if((CURLE_OK != res) || (code!=200)) {
        curl_easy_cleanup(curl);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_RESPONSE_CODE);
    }

    curl_easy_cleanup(curl);
    return speed_test_create_result(chunk);
}

SpeedTestResult speed_test_parse_config(MemoryChunk *xml) {
    xmlDocPtr doc = xmlReadMemory(xml->memory, (int)xml->size, NULL, NULL, NULL);
    if(NULL == doc) {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_READ);
    }

    xmlNodePtr  root = xmlDocGetRootElement(doc);
    if (NULL == root) {
        xmlFreeDoc(doc);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_EMPTY_DOC);
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "settings")) {
        xmlFreeDoc(doc);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_INVALID_FILE);
    }

    xmlNodePtr child = root->xmlChildrenNode;
    while (child && (xmlIsBlankNode(child) || (xmlStrcmp(child->name, (const xmlChar *) "client")) != 0)) {
        child = child->next;
    }

    if (child == NULL) {
        //uu nic niema konczymy
        xmlFreeDoc(doc);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_NO_CHILD);
    }

    if (xmlStrcmp(child->name, (const xmlChar *) "client") != 0) {
        xmlFreeDoc(doc);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_NOT_FOUND_ELEMENT);
    }

    SpeedTestResult speedTestResult = speed_test_parse_client_element(child);

    if (!speed_test_has_error(&speedTestResult)) {
        SpeedTestClient *client = speedTestResult.data;
        SpeedTestConfig *config = (SpeedTestConfig *)malloc(sizeof(SpeedTestConfig));

        if (NULL == config) {
            speed_test_free_config_client(client);
            xmlFreeDoc(doc);
            return speed_test_create_result_posix_error(ENOMEM);
        }

        memset(config, 0, sizeof(SpeedTestConfig));
        config->client = client;
        xmlFreeDoc(doc);
        return speed_test_create_result(config);
    } else {
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_CANT_PARSE_CONFIG_RESPONSE);
    }
}

double speed_test_util_degrees_to_radians(double angleDegrees) {
    return angleDegrees * M_PI / 180.0;
}

double speed_test_util_distance(SpeedTestCoordinate *c1, SpeedTestCoordinate *c2) {
    int radius = 6371; //km

    double dlat = speed_test_util_degrees_to_radians(c2->lat - c1->lat);
    double dlon = speed_test_util_degrees_to_radians(c2->lon - c1->lon);

    double a = (sin(dlat / 2) * sin(dlat / 2) +
                cos(speed_test_util_degrees_to_radians(c1->lat)) *
                cos(speed_test_util_degrees_to_radians(c2->lat)) * sin(dlon / 2) *
                sin(dlon / 2));

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double d = radius * c;

    return d;
}


SpeedTestResult speed_test_closest_servers(SpeedTestCoordinate *coordinate, SpeedTestServersList *servers) {
    SpeedTestServersList *head = servers;
    while (servers->next != NULL) {
        if (servers->s != NULL) {
            SpeedTestCoordinate cord2 = {servers->s->lat, servers->s->lon};
            servers->s->distance = speed_test_util_distance(coordinate, &cord2);
        }

        servers = servers->next;
    }

    //zwracamy 5 najblizszych serwerow
    SpeedTestResult speedTestResult = speed_test_get_best_servers(head, 5);
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    SpeedTestServersList * serverLinkList1 = speedTestResult.data;
    return speed_test_create_result(serverLinkList1);
}


char *speed_test_get_api_data(SpeedTestApi *data) {
    char *buf = (char*)malloc(500);
    char *buf2 = (char*)malloc(500);

    if (NULL == buf || NULL == buf2) {
        free(buf);
        free(buf2);
        return NULL;
    }

    memset(buf2, 0, 500);

    snprintf(buf2, 500, "%u-%u-%u-%s", data->ping, data->upload, data->download, "297aae72");
    char *hash = md5(buf2, strlen(buf2));

    snprintf(buf, 500, "startmode=recommendedselect&promo=&upload=%u&accuracy=8&recommendedserverid=%d"
                     "&serverid=%d&ping=%u&hash=%s&download=%u",
             data->upload, data->recommendedserverid, data->serverid, data->ping, hash, data->download
    );
    free(buf2);
    free(hash);

    return buf;
}

SpeedTestResult speed_test_request_share_result(SpeedTestApi *speedTestApi) {
    struct MemoryChunk *chunk = (MemoryChunk *)malloc(sizeof(MemoryChunk));
    if (chunk == NULL) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    chunk->memory = (char*)malloc(1024);  /* will be grown as needed by the realloc above */
    chunk->size = 0;    /* no data at this point */
    chunk->allocated = 1024;    /* no data at this point */
    if (chunk->memory == NULL) {
        speed_test_free_memory_chunk(chunk);
        return speed_test_create_result_posix_error(ENOMEM);
    }

    CURL *curl = curl_easy_init();
    CURLcode res;

    if (!curl) {
        speed_test_free_memory_chunk(chunk);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_CURL_INIT);
    }

    char *post = speed_test_get_api_data(speedTestApi);
    if (NULL == post) {
        speed_test_free_memory_chunk(chunk);
        curl_easy_cleanup(curl);
        return speed_test_create_result_posix_error(ENOMEM);
    }

    char *url = "https://www.speedtest.net/api/api.php";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "speedtest/0.1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post));
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Referer: http://c.speedtest.net/flash/speedtest.swf");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        curl_slist_free_all(headers); /* free the list again */
        curl_easy_cleanup(curl);
        speed_test_free_memory_chunk(chunk);
        return speed_test_create_result_curl_error(res);
    }

    curl_slist_free_all(headers); /* free the list again */

    int code = 0;
    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if((CURLE_OK != res) || (code!=200)) {
        curl_easy_cleanup(curl);
        speed_test_free_memory_chunk(chunk);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_RESPONSE_CODE);
    }

    free(post);
    curl_easy_cleanup(curl);
    return speed_test_create_result(chunk);
}

SpeedTestResult speed_test_parse_share_result(MemoryChunk *memoryStruct) {
    SpeedTestApiResponse *response = (SpeedTestApiResponse *) malloc(sizeof(SpeedTestApiResponse));
    if (NULL == response) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    char *saveptr1, *saveptr2;

    char *var;
    char *ptr = strtok_r(memoryStruct->memory, "&", &saveptr1);
    while (NULL != ptr) {
        char *key = strtok_r(ptr, "=", &saveptr2);
        if (NULL != key) {
            char *val = strtok_r(NULL, "=", &saveptr2);

            //todo mmo co z kodowaniem
            var = val;

            if (strcmp(key, "resultid") == 0) {
                response->resultid = atoll(var);
            } else if (strcmp(key, "date") == 0) {
                response->date = strdup(var);
            } else if (strcmp(key, "rating") == 0) {
                response->rating = atoi(var);
            } else if (strcmp(key, "time") == 0) {
                response->time = strdup(var);
            }

        }

        ptr = strtok_r(NULL, "&", &saveptr1);
    }

    return speed_test_create_result(response);
}


SpeedTestResult speed_test_parse_client_element(xmlNodePtr node) {
    SpeedTestClient *client = (SpeedTestClient *)malloc(sizeof(SpeedTestClient));
    if (NULL == client) {
        return speed_test_create_result_posix_error(ENOMEM);
    }
    memset(client, 0, sizeof(SpeedTestClient));

    if (xmlStrcmp(node->name, (const xmlChar*)"client") == 0) {
        client->ip = xmlGetProp(node, (const xmlChar*)"ip");
        char *lat = (char*)xmlGetProp(node, (const xmlChar*)"lat");
        char *lon = (char*)xmlGetProp(node, (const xmlChar*)"lon");
        client->lat = atof(lat);
        client->lon = atof(lon);
        client->isp = xmlGetProp(node, (const xmlChar*)"isp");

        free(lat);
        free(lon);

        return speed_test_create_result(client);
    } else {
        //wczesniej nadpisujemy dane struktury zerami wiec nie ma bledu
        speed_test_free_config_client(client);
        return speed_test_create_result_posix_error(SPEED_TEST_ERROR_XML_INVALID_ELEMENT);
    }
}



void speed_test_free_memory_chunk(MemoryChunk *memoryChunk) {
    free(memoryChunk->memory);
    memoryChunk->allocated = 0;
    memoryChunk->size = 0;
    free(memoryChunk);
}

void speed_test_free_config(SpeedTestConfig *config) {
    speed_test_free_config_client(config->client);
    free(config);
}

void speed_test_free_config_client(SpeedTestClient *client) {
    free(client->ip);
    free(client->isp);
}


void speed_test_free_server(SpeedTestServer *server) {
    free(server->cc);
    free(server->country);
    free(server->host);
    free(server->name);
    free(server->sponsor);
    free(server->url);
    free(server->url2);
    free(server);
}

void *speed_test_memdup(void *src, size_t  size) {
    void *tmp = malloc(size);
    if (tmp == NULL) {
        return NULL;
    }
    memset(tmp, 0, size);
    memcpy(tmp, src, size);
    return tmp;
}

void speed_test_free_api_share_response(SpeedTestApiResponse * apiShareResponse)
{
    free(apiShareResponse->date);
    free(apiShareResponse->time);
    free(apiShareResponse);
}

SpeedTestResult speed_test_copy_server(SpeedTestServer *server) {
    SpeedTestServer* copy = (SpeedTestServer *)malloc(sizeof(SpeedTestServer));
    if (NULL == copy) {
        return speed_test_create_result_posix_error(ENOMEM);
    }


    if (server->sponsor != NULL) {
        copy->sponsor = speed_test_memdup(server->sponsor, (size_t)xmlStrlen(server->sponsor)+1);
        if (NULL == copy->sponsor) {
            speed_test_free_server(copy);
            return speed_test_create_result_posix_error(ENOMEM);
        }
    } else {
        copy->sponsor = NULL;
    }


    if (server->name != NULL) {
        copy->name = speed_test_memdup(server->name, (size_t)xmlStrlen(server->name)+1);
        if (NULL == copy->name) {
            speed_test_free_server(copy);
            return speed_test_create_result_posix_error(ENOMEM);
        }
    } else {
        copy->name = NULL;
    }


    if (server->host != NULL) {
        copy->host = speed_test_memdup(server->host, (size_t)xmlStrlen(server->host)+1);
        if (NULL == copy->host) {
            speed_test_free_server(copy);
            return speed_test_create_result_posix_error(ENOMEM);
        }
    } else {
        copy->host = NULL;
    }


    if (server->cc != NULL) {
        copy->cc = speed_test_memdup(server->cc, (size_t)xmlStrlen(server->cc)+1);
        if (NULL == copy->cc) {
            speed_test_free_server(copy);
            return speed_test_create_result_posix_error(ENOMEM);
        }
    } else {
        copy->cc = NULL;
    }


    if (server->country != NULL) {
        copy->country = speed_test_memdup(server->country, (size_t)xmlStrlen(server->country)+1);
        if (NULL == copy->country) {
            speed_test_free_server(copy);
            return speed_test_create_result_posix_error(ENOMEM);
        }
    } else {
        copy->country = NULL;
    }

    copy->distance = server->distance;
    copy->id = server->id;
    copy->lat = server->lat;
    copy->lon = server->lon;
    copy->latency = server->latency;

    if (server->url != NULL) {
        copy->url = speed_test_memdup(server->url, (size_t)xmlStrlen(server->url)+1);
        if (NULL == copy->url) {
            speed_test_free_server(copy);
            return speed_test_create_result_posix_error(ENOMEM);
        }
    } else {
        copy->url = NULL;
    }


    if (server->url2 != NULL) {
        copy->url2 = speed_test_memdup(server->url2, (size_t)xmlStrlen(server->url2)+1);
        if (NULL == copy->url2) {
            speed_test_free_server(copy);
            return speed_test_create_result_posix_error(ENOMEM);
        }
    } else {
        copy->url2 = NULL;
    }

    return speed_test_create_result(copy);
}

void speed_test_set_min_value(SpeedTestStatic *speedTestStatic, double min)
{
    if (speedTestStatic->min > min || speedTestStatic->min == 0) {
        speedTestStatic->min = min;
    }
}
void speed_test_set_max_value(SpeedTestStatic *speedTestStatic, double max)
{
    if (speedTestStatic->max < max) {
        speedTestStatic->max = max;
    }
}
void speed_test_set_avg_value(SpeedTestStatic *speedTestStatic, double avg)
{
    speedTestStatic->avg = avg;
}

SpeedTestServer* speed_test_create_server() {
    SpeedTestServer *ret = NULL;
    ret = (SpeedTestServer *) malloc(sizeof(SpeedTestServer));

    if (NULL != ret) {
        memset(ret, 0, sizeof(SpeedTestServer));
    }

    return ret;
}

SpeedTestStatic* speed_test_create_static() {
    SpeedTestStatic *speedTestStatic = (SpeedTestStatic*)malloc(sizeof(SpeedTestStatic));
    if (NULL == speedTestStatic) {
        perror("No memory.");
        return NULL;
    }
    speedTestStatic->avg = 0;
    speedTestStatic->min = 0;
    speedTestStatic->max = 0;

    return speedTestStatic;
}
SpeedTestResult speed_test_get_server_by_id(SpeedTestServersList *list, int id) {
    SpeedTestResult speedTestResult;
    SpeedTestServer *found = NULL;
    while (list != NULL) {
        if (list->s != NULL && list->s->id == id) {
            speedTestResult = speed_test_copy_server(list->s);
            if (speed_test_has_error(&speedTestResult)) {
                return speedTestResult;
            } else {
                found = speedTestResult.data;
            }
            break;
        }
        list = list->next;
    }

    return speed_test_create_result(found);
}

SpeedTestResult speed_test_get_config() {
    SpeedTestResult speedTestResult;
    speedTestResult = speed_test_request_get_config();
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    MemoryChunk *configXml = speedTestResult.data;

    speedTestResult = speed_test_parse_config(configXml);
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }
    speed_test_free_memory_chunk(configXml);

    SpeedTestConfig *config = speedTestResult.data;
    return speed_test_create_result(config);
}

SpeedTestResult speed_test_get_servers_list() {
    SpeedTestResult speedTestResult = speed_test_request_get_servers_list();
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    MemoryChunk *xml = speedTestResult.data;
    speedTestResult = speed_test_parse_servers_list(xml);
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    speed_test_free_memory_chunk(xml);
    SpeedTestServersList *test = speedTestResult.data;

    return speed_test_create_result(test);
}

SpeedTestResult speed_test_get_share_result(SpeedTestApi* speedTestApi) {
    SpeedTestResult speedTestResult = speed_test_request_share_result(speedTestApi);
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    MemoryChunk *mem = speedTestResult.data;
    speedTestResult = speed_test_parse_share_result(mem);
    if (speed_test_has_error(&speedTestResult)) {
        return speedTestResult;
    }

    SpeedTestApiResponse *apiShareResponse = speedTestResult.data;
    speed_test_free_memory_chunk(mem);
    return speed_test_create_result(apiShareResponse);
}