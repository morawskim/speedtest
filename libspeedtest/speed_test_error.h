//
// Created by marcin on 26.06.15.
//

#ifndef CSPEEDTESTCLION_ERROR_H
#define CSPEEDTESTCLION_ERROR_H

#include "speedtest.h"

#define SPEED_TEST_ERROR_XML_READ 256
#define SPEED_TEST_ERROR_XML_EMPTY_DOC 257
#define SPEED_TEST_ERROR_XML_INVALID_FILE 258
#define SPEED_TEST_ERROR_XML_NO_CHILD 259
#define SPEED_TEST_ERROR_XML_NOT_FOUND_ELEMENT 260
#define SPEED_TEST_ERROR_XML_INVALID_ELEMENT 261
#define SPEED_TEST_ERROR_CANT_PARSE_CONFIG_RESPONSE 262
#define SPEED_TEST_ERROR_RESPONSE_CODE 263
#define SPEED_TEST_ERROR_CURL_INIT 264
#define SPEED_TEST_ERROR_RANDOM_DATA 265


typedef enum SpeedTestErrorType {
    SPEED_TEST_ERROR_TYPE_POSIX,
    SPEED_TEST_ERROR_TYPE_CURL_ERROR
} SpeedTestErrorType;

typedef union SpeedTestErrorUnion {
    int errno;
    CURLcode curl_code;
} SpeedTestErrorUnion;

typedef struct SpeedTestError {
    union SpeedTestErrorUnion error;
    SpeedTestErrorType error_type;
} SpeedTestError;

typedef struct SpeedTestResult {
    SpeedTestError error;
    short hasError;
    void* data;
} SpeedTestResult;


int speed_test_has_error(SpeedTestResult* speedTestResult);

const char* speed_test_error_message(SpeedTestError speedTestError);

SpeedTestResult speed_test_create_result_curl_error(CURLcode curLcode);
SpeedTestResult speed_test_create_result_posix_error(int error);
SpeedTestResult speed_test_create_result(void* data);

#endif //CSPEEDTESTCLION_ERROR_H
