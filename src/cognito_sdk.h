#pragma once

struct CognitoRequest;
struct CognitoResponse;

CognitoResponse cognito_sdk_post(const CognitoRequest &request);
