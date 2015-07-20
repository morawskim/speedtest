#ifndef CSPEEDTESTCLION_CLI_H
#define CSPEEDTESTCLION_CLI_H

#include "libspeedtest/speedtest.h"

typedef struct AppConfig {
    short quiet;
    short sharing;
    short humanUnits;
    int serverId;
    int listServers;
    int download;
    int upload;
    int latency;
} AppConfig;

typedef struct SpeedTestHumanUnit {
    double value;
    char unit;
    const char* s;
} SpeedTestHumanUnit;

void speed_test_human_unit_free(SpeedTestHumanUnit* speedTestHumanUnit);

SpeedTestHumanUnit* speed_test_human_units(double bytes);

void speed_test_print_static(SpeedTestStatic *speedTestStatic, int humanValue);

void displayConfig(SpeedTestConfig *config);

void speed_test_print_server_data(SpeedTestServer * server);
void speed_test_print_servers_data(SpeedTestServersList *list);

SpeedTestResult speed_test_test_upload(SpeedTestServer *server);
SpeedTestResult speed_test_test_download(SpeedTestServer *server);
SpeedTestResult speed_test_test_latency(SpeedTestServer *s);

#endif //CSPEEDTESTCLION_CLI_H
