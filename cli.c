#include <asm-generic/errno-base.h>
#include "cli.h"

void speed_test_human_unit_free(SpeedTestHumanUnit* speedTestHumanUnit) {
    if (NULL != speedTestHumanUnit) {
        free((char*)speedTestHumanUnit->s);
        free(speedTestHumanUnit);
    }
}

SpeedTestHumanUnit* speed_test_human_units(double bytes) {
    SpeedTestHumanUnit* humanUnit = (SpeedTestHumanUnit*)malloc(sizeof(SpeedTestHumanUnit));
    if (NULL == humanUnit) {
        perror("no memory");
        return NULL;
    }

    char* s = (char*)malloc(4);
    memset(s, 0, 4);

    int unit = 1024;
    if (bytes < unit) {
        humanUnit->value = bytes;
        humanUnit->unit = 'B';
        humanUnit->s = s;
        snprintf(s, 4, "B");
        return humanUnit;
    }

    int exp = (int)(log(bytes) / log(unit));
    char pre[6] = {'K', 'M', 'G', 'T', 'P', "E"};
    double u = bytes / pow(unit, exp);


    humanUnit->unit = pre[exp-1];
    humanUnit->value = u;
    humanUnit->s = s;
    snprintf(s, 4, "%ciB", humanUnit->unit);
    return humanUnit;
}

void speed_test_print_static(SpeedTestStatic *speedTestStatic, int humanValue) {
    if (humanValue) {
        //todo mmo obsluga NULLi
        SpeedTestHumanUnit* humanMin = speed_test_human_units(speedTestStatic->min);
        SpeedTestHumanUnit* humanAvg = speed_test_human_units(speedTestStatic->avg);
        SpeedTestHumanUnit* humanMax = speed_test_human_units(speedTestStatic->max);

        if (NULL != humanMin && NULL != humanAvg && NULL != humanMax) {
            printf("MIN:%0.3f%s AVG:%0.3f%s MAX:%0.3f%s\n",
                   humanMin->value, humanMin->s,
                   humanAvg->value, humanAvg->s,
                   humanMax->value, humanMax->s
            );
        }
        speed_test_human_unit_free(humanMin);
        speed_test_human_unit_free(humanAvg);
        speed_test_human_unit_free(humanMax);

    } else {
        printf("MIN:%0.3f AVG:%0.3f MAX:%0.3f\n",
               speedTestStatic->min,
               speedTestStatic->avg,
               speedTestStatic->max
        );
    }

}

void displayConfig(SpeedTestConfig *config) {
    printf("ISP: %s", (char*)config->client->isp);
    printf("LAT: %0.3f ", config->client->lat);
    printf("LON: %0.3f ", config->client->lon);
    printf("\n");

}

void speed_test_print_server_data(SpeedTestServer * server) {
    printf("Server '#%d' '%s' sponsored by '%s' located in '%s' (lat: %0.2f, lon: %0.2f)\n",
           server->id,
           server->name,
           server->sponsor,
           server->country,
           server->lat,
           server->lon
    );
}

void speed_test_print_servers_data(SpeedTestServersList *list) {
    while (list != NULL) {
        if (NULL != list->s) {
            speed_test_print_server_data(list->s);
        }
        list = list->next;
    }
}

SpeedTestResult speed_test_test_upload(SpeedTestServer *server) {
    static const int ile2 = 4;
    //1kilo 100kilo 2mb 10mb
    size_t sizes2[ile2];
    sizes2[0] = 102400;
    sizes2[1] = 204800;
    sizes2[2] = 1048576;
    sizes2[3] = 1048576;
//    sizes2[3] = 10485760;

//    sizes2[0] = 1048576;
//    sizes2[1] = 1048576;

    SpeedTestStatic *speedTestStatic = speed_test_create_static();
    if (NULL == speedTestStatic) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    double speed = 0;
    double total = 0.0;

    for (int i=0; i<ile2; i++) {
        for (int j=0; j<3; j++) {
            char *randomData = getRandomData(sizes2[i]);
            if (NULL == randomData) {
                free(speedTestStatic);
                return speed_test_create_result_posix_error(SPEED_TEST_ERROR_RANDOM_DATA);
            }

            SpeedTestResult speedTestResult = speed_test_upload_file((char*)server->url, randomData, sizes2[i]);
            if (speed_test_has_error(&speedTestResult)) {
                return speedTestResult;
            }

            SpeedTestCurlStatics *statics = speedTestResult.data;
            speed += statics->speed;
            free(randomData);

            speed_test_set_max_value(speedTestStatic, statics->speed);
            speed_test_set_min_value(speedTestStatic, statics->speed);

            free(statics);
        }
        speed = speed/3;
        total += speed;
        speed = 0.0;
    }
    speed_test_set_avg_value(speedTestStatic, total/ile2);
    return speed_test_create_result(speedTestStatic);
}

SpeedTestResult speed_test_test_download(SpeedTestServer *server) {
    static const int ile2 = 4;
    int sizes2[ile2];
    sizes2[0] = 500;
    sizes2[1] = 1500;
    sizes2[2] = 2500;
    sizes2[3] = 4000;

    sizes2[0] = 3500;
    sizes2[1] = 3500;

    double speed = 0.0;
    double total = 0.0;
    SpeedTestStatic *speedTestStatic = speed_test_create_static();
    if (NULL == speedTestStatic) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    for (int i=0; i<ile2; i++) {
        int c = 0;
        for (int j=0; j<3; j++) {
            char *imageUrl = speed_test_url_image((char*)server->url, sizes2[i]);
            if (NULL == imageUrl) {
                free(speedTestStatic);
                return speed_test_create_result_posix_error(ENOMEM);
            }

            SpeedTestResult speedTestResult = speed_test_download_image(imageUrl);
            if (speed_test_has_error(&speedTestResult)) {
                free(imageUrl);
                free(speedTestStatic);
                return speedTestResult;
            }

            SpeedTestCurlStatics *statics = speedTestResult.data;
            speed += statics->speed;

            speed_test_set_max_value(speedTestStatic, statics->speed);
            speed_test_set_min_value(speedTestStatic, statics->speed);
            c++;
            free(statics);
            free(imageUrl);
        }
        speed = speed/c;
        total += speed;
        speed = 0.0;
    }
    speed_test_set_avg_value(speedTestStatic, total/ile2);
    return speed_test_create_result(speedTestStatic);
}

SpeedTestResult speed_test_test_latency(SpeedTestServer *s) {
    char *latencyUrl = speed_test_url_latency((char*)s->url);
    double latency = 0.0;
    double avg = 0.0;
    const int iterations = 3;

    SpeedTestStatic* speedTestStatic = speed_test_create_static();
    if (NULL == speedTestStatic) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    if (NULL == latencyUrl) {
        return speed_test_create_result_posix_error(ENOMEM);
    }

    SpeedTestCurlStatics* curlStatics;
    for (int i=0; i<iterations; i++) {
        SpeedTestResult speedTestResult = speed_test_get_latency(latencyUrl);
        if (speed_test_has_error(&speedTestResult)) {
            return speedTestResult;
        }

        curlStatics = speedTestResult.data;

        latency += curlStatics->time;
        speed_test_set_max_value(speedTestStatic, curlStatics->time);
        speed_test_set_min_value(speedTestStatic, curlStatics->time);
        free(curlStatics);
    }

    free(latencyUrl);

    avg = latency/iterations;
    speed_test_set_avg_value(speedTestStatic, avg);
    s->latency = avg;

    return speed_test_create_result(speedTestStatic);
}
