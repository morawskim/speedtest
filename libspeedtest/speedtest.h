#ifndef CSPEEDTESTCLION_SPEED_TEST_H
#define CSPEEDTESTCLION_SPEED_TEST_H

#define _GNU_SOURCE 1
//#define __extern_always_inline 1

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libgen.h>
#include <math.h>
#include "util.h"
#include "md5.h"
#include <error.h>
#include "speed_test_error.h"

typedef struct SpeedTestServer {
    xmlChar *url;
    double lat;
    double lon;
    xmlChar *name;
    xmlChar *country;
    xmlChar *cc;
    xmlChar *sponsor;
    int id;
    xmlChar *url2;
    xmlChar *host;

    double latency;
    double distance;
} SpeedTestServer;

typedef struct SpeedTestServersList {
    SpeedTestServer *s;
    struct SpeedTestServersList *next;
} SpeedTestServersList;

typedef  struct SpeedTestCurlStatics {
    double bytesDownload;
    double speed;
    double time;
    long code;
} SpeedTestCurlStatics;

typedef struct SpeedTestClient {
    double lat;
    double lon;
    xmlChar* ip;
    xmlChar* isp;
} SpeedTestClient;

typedef struct SpeedTestConfig {
    SpeedTestClient *client;
} SpeedTestConfig;

typedef struct SpeedTestCoordinate {
    double lat;
    double lon;
} SpeedTestCoordinate;

typedef  struct SpeedTestApi {
    unsigned int download;
    unsigned int ping;
    unsigned int upload;
    char *promo;
    char *startmode;
    int recommendedserverid;
    short accuracy;
    int serverid;
    char *hash;
} SpeedTestApi;

typedef struct SpeedTestApiResponse {
    long long resultid;
    char *date;
    char *time;
    int rating;
} SpeedTestApiResponse;

typedef struct SpeedTestStatic {
    double min;
    double max;
    double avg;
} SpeedTestStatic;

typedef struct SpeedTestResult SpeedTestResult;


SpeedTestResult speed_test_request_get_servers_list();
SpeedTestResult speed_test_request_get_config();
SpeedTestResult speed_test_request_share_result(SpeedTestApi *speedTestApi);

SpeedTestResult speed_test_parse_servers_list(MemoryChunk *xml);
SpeedTestResult speed_test_parse_server_node(xmlDocPtr doc, xmlNodePtr cur);
SpeedTestResult speed_test_parse_share_result(MemoryChunk *memoryStruct);
SpeedTestResult speed_test_parse_client_element(xmlNodePtr node);
SpeedTestResult speed_test_parse_config(MemoryChunk *xml);


int speed_test_best_server_compare(const void *a, const void *b);

char* speed_test_url_latency(char *uploadUrl);
char* speed_test_url_image(char *uploadUrl, int imgSize);





SpeedTestResult speed_test_download_image(char *imgUrl);
void speed_test_print_statics(SpeedTestCurlStatics *statics);
SpeedTestResult speed_test_get_best_servers(SpeedTestServersList *list, const size_t max);
SpeedTestResult speed_test_upload_file(char *url, char *binaryString, long size);
SpeedTestResult speed_test_get_latency(char *url);
SpeedTestServer *speed_test_create_empty_server_struct();
int speed_test_is_empty_server_struct(SpeedTestServer *srv);
SpeedTestResult speed_test_get_best_server(SpeedTestServersList *list);
SpeedTestResult speed_test_closest_servers(SpeedTestCoordinate *coordinate, SpeedTestServersList *servers);
char *speed_test_get_api_data(SpeedTestApi *data);

double speed_test_util_degrees_to_radians(double angleDegrees);
double speed_test_util_distance(SpeedTestCoordinate* c1, SpeedTestCoordinate* c2);


void speed_test_free_server_link_list(SpeedTestServersList* list);
void speed_test_free_memory_chunk(MemoryChunk* memoryChunk);
void speed_test_free_config(SpeedTestConfig* config);
void speed_test_free_config_client(SpeedTestClient* client);
void speed_test_free_server(SpeedTestServer* server);
void speed_test_free_api_share_response(SpeedTestApiResponse* apiShareResponse);

void speed_test_set_min_value(SpeedTestStatic *speedTestStatic, double min);
void speed_test_set_max_value(SpeedTestStatic *speedTestStatic, double max);
void speed_test_set_avg_value(SpeedTestStatic *speedTestStatic, double avg);

SpeedTestServer* speed_test_create_server();
SpeedTestStatic* speed_test_create_static();
SpeedTestResult speed_test_copy_server(SpeedTestServer *server);
SpeedTestResult speed_test_get_server_by_id(SpeedTestServersList *list, int id);

SpeedTestResult speed_test_get_config();
SpeedTestResult speed_test_get_servers_list();
SpeedTestResult speed_test_get_share_result(SpeedTestApi* speedTestApi);

#endif //CSPEEDTESTCLION_SPEED_TEST_H
