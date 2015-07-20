//
// Created by marcin on 26.06.15.
//

#include "speed_test_error.h"
//#include <errno.h>


int speed_test_has_error(SpeedTestResult* speedTestResult) {
    if (NULL == speedTestResult) {
        return 1;
    } else {
        return speedTestResult->hasError;
    }
}

const char* speed_test_error_message(SpeedTestError speedTestError) {
    if (speedTestError.error_type == SPEED_TEST_ERROR_TYPE_CURL_ERROR) {
        return strerror(speedTestError.error.errno);
    } else if (speedTestError.error_type == SPEED_TEST_ERROR_TYPE_POSIX) {
        return curl_easy_strerror(speedTestError.error.curl_code);
    } else {
        return NULL;
    }
}

const char* speed_test_result_error_message(SpeedTestResult* speedTestResult) {
    if (NULL == speedTestResult) {
        return NULL;
    } else {
        if (speed_test_has_error(speedTestResult)) {
            return  speed_test_error_message(speedTestResult->error);
        } else {
            return NULL;
        }
    }
}

SpeedTestResult speed_test_create_result_curl_error(CURLcode curLcode) {
    SpeedTestResult speedTestResult;
    speedTestResult.hasError = 1;
    speedTestResult.data = NULL;
    speedTestResult.error.error_type = SPEED_TEST_ERROR_TYPE_CURL_ERROR;
    speedTestResult.error.error.curl_code = curLcode;
    return speedTestResult;
}
SpeedTestResult speed_test_create_result_posix_error(int error) {
    SpeedTestResult speedTestResult;
    speedTestResult.hasError = 1;
    speedTestResult.data = NULL;
    speedTestResult.error.error_type = SPEED_TEST_ERROR_TYPE_POSIX;
    speedTestResult.error.error.errno = error;
    return speedTestResult;
}

SpeedTestResult speed_test_create_result(void* data) {
    SpeedTestResult speedTestResult;
    speedTestResult.hasError = 0;
    speedTestResult.data = data;
    return speedTestResult;
}