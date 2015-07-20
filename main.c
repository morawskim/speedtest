#include <getopt.h>
#include "libspeedtest/speedtest.h"
#include "cli.h"

int main(int argc, char** argv) {
    curl_global_init(CURL_GLOBAL_ALL);

    AppConfig appConfig;
    appConfig.humanUnits = 0;
    appConfig.serverId = 0;
    appConfig.quiet = 0;
    appConfig.sharing = 0;
    appConfig.listServers = 0;
    appConfig.latency = 0;
    appConfig.download = 0;
    appConfig.upload = 0;

    int opt;
    while ((opt = getopt (argc, argv, "pqhls:")) != -1) {
        switch (opt) {
            case 'p':
                appConfig.sharing = 1;
                break;
            case 'q':
                appConfig.quiet = 1;
                break;
            case 'h':
                appConfig.humanUnits = 1;
                break;
            case 'l':
                appConfig.listServers = 1;
                break;
            case 'd':
                appConfig.download = 1;
                break;
            case 'u':
                appConfig.upload = 1;
                break;
            case 'g':
                appConfig.latency = 1;
                break;
            case 's':
                appConfig.serverId = atoi(optarg);
                break;
            case '?':
                if (optopt == 's')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                             "Unknown option character `\\x%x'.\n",
                             optopt);
                return 1;
            default:
                break;
        }
    }

    if (appConfig.sharing && (appConfig.latency || appConfig.upload || appConfig.download)) {
        fprintf(stderr, "Cant publis result if only one value will be testing");
        exit(EXIT_FAILURE);
    }

    int simpleMode = 0;
    if (appConfig.latency || appConfig.upload || appConfig.download) {
        simpleMode = 1;
    }

    SpeedTestResult speedTestResult = speed_test_get_config();
    if (speed_test_has_error(&speedTestResult)) {
        fprintf(stderr, "Cant get config:");
        fprintf(stderr, speed_test_error_message(speedTestResult.error));
        exit(EXIT_FAILURE);
    }
    SpeedTestConfig *config = speedTestResult.data;
    displayConfig(config);

    speedTestResult = speed_test_get_servers_list();
    if (speed_test_has_error(&speedTestResult)) {
        fprintf(stderr, "Cant get servers list:");
        fprintf(stderr, speed_test_error_message(speedTestResult.error));
        exit(EXIT_FAILURE);
    }
    SpeedTestServersList* test = speedTestResult.data;

    if (appConfig.listServers) {
        speed_test_print_servers_data(test);
        speed_test_free_config(config);
        speed_test_free_server_link_list(test);
        exit(EXIT_SUCCESS);
    }

    SpeedTestCoordinate c = {config->client->lat, config->client->lon};

    SpeedTestServer *best = NULL;
    if (appConfig.serverId) {
        speedTestResult = speed_test_get_server_by_id(test, appConfig.serverId);
        if (speed_test_has_error(&speedTestResult)) {
            fprintf(stderr, "Cant get server by id");
            fprintf(stderr, speed_test_error_message(speedTestResult.error));
            exit(EXIT_FAILURE);
        } else if (NULL == speedTestResult.data) {
            fprintf(stderr, "Server '#%d' not exist", appConfig.serverId);
            exit(EXIT_FAILURE);
        } else {
            best = speedTestResult.data;
        }
    } else {
        speedTestResult = speed_test_closest_servers(&c, test);
        if (speed_test_has_error(&speedTestResult)) {
            fprintf(stderr, "Cant get closest servers");
            fprintf(stderr, speed_test_error_message(speedTestResult.error));
            exit(EXIT_FAILURE);
        }

        SpeedTestServersList * serverLinkList1 = speedTestResult.data;

        speedTestResult = speed_test_get_best_server(serverLinkList1);
        if (speed_test_has_error(&speedTestResult)) {
            fprintf(stderr, "Cant get best server from closest servers");
            fprintf(stderr, speed_test_error_message(speedTestResult.error));
            exit(EXIT_FAILURE);
        }

        best = speedTestResult.data;
        speed_test_free_server_link_list(serverLinkList1);
    }

    //latency
    SpeedTestStatic*avgLatency = NULL;
    double l = 0.0;
    if (appConfig.latency || !simpleMode) {
        speedTestResult = speed_test_test_latency(best);
        if (speed_test_has_error(&speedTestResult)) {
            fprintf(stderr, "Get latency error");
            fprintf(stderr, speed_test_error_message(speedTestResult.error));
            exit(EXIT_FAILURE);
        }

        avgLatency = speedTestResult.data;
        l = avgLatency->avg;
        free(avgLatency);
    }

    //upload
    SpeedTestStatic* avgUploadSpeed = NULL;
    if (appConfig.upload || !simpleMode) {
        speedTestResult = speed_test_test_upload(best);
        if (speed_test_has_error(&speedTestResult)) {
            fprintf(stderr, "Test upload error");
            fprintf(stderr, speed_test_error_message(speedTestResult.error));
            exit(EXIT_FAILURE);
        }

        printf("Upload: ");
        avgUploadSpeed = speedTestResult.data;
        speed_test_print_static(avgUploadSpeed, appConfig.humanUnits);
    }

    //download
    SpeedTestStatic* avgDownloadSpeed = NULL;
    if (appConfig.download || !simpleMode) {
        speedTestResult = speed_test_test_download(best);
        if (speed_test_has_error(&speedTestResult)) {
            fprintf(stderr, "Test download error");
            fprintf(stderr, speed_test_error_message(speedTestResult.error));
            exit(EXIT_FAILURE);
        }
        avgDownloadSpeed = speedTestResult.data;
        printf("Download: ");
        speed_test_print_static(avgDownloadSpeed, appConfig.humanUnits);
    }
    printf("\n\n\n\n");

    if (!simpleMode) {
        if (appConfig.sharing) {
            SpeedTestApi speedTestApi;
            speedTestApi.download = (unsigned int) (avgDownloadSpeed->avg) / 1024;
            speedTestApi.upload = (unsigned int) (avgUploadSpeed->avg) / 1024;
            speedTestApi.serverid = best->id;
            speedTestApi.recommendedserverid = best->id;
            speedTestApi.ping = (unsigned int) (l * 1000);

            speedTestResult = speed_test_get_share_result(&speedTestApi);
            if (speed_test_has_error(&speedTestResult)) {
                fprintf(stderr, "Cant share results");
                fprintf(stderr, speed_test_error_message(speedTestResult.error));
                exit(EXIT_FAILURE);
            }

            SpeedTestApiResponse *apiShareResponse = speedTestResult.data;
            printf("Image share:\n");
            printf("http://www.speedtest.net/result/%lld.png", apiShareResponse->resultid);
            printf("\n");
            printf("http://www.speedtest.net/my-result/%lld", apiShareResponse->resultid);
            printf("\n");

            speed_test_free_api_share_response(apiShareResponse);
        }
    }

    free(avgDownloadSpeed);
    free(avgUploadSpeed);

    speed_test_free_config(config);
    speed_test_free_server_link_list(test);
    speed_test_free_server(best);
    curl_global_cleanup();

    return 0;
}