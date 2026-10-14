#include "cognito_client.h"

#include <memory>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#elif defined(__ANDROID__)
#include <SDL3/SDL.h>
#include <jni.h>
#else
#include <curl/curl.h>
#endif

namespace
{
constexpr std::size_t MAX_RESPONSE_SIZE = 1024 * 1024;
#if defined(_WIN32)
struct CloseInternet
{
    void operator()(void *handle) const
    {
        WinHttpCloseHandle(handle);
    }
};
using InternetHandle = std::unique_ptr<void, CloseInternet>;

InternetHandle checked(HINTERNET handle)
{
    if (handle == nullptr)
        throw std::runtime_error("Unable to connect to Cognito. Check your network.");
    return InternetHandle(handle);
}
#elif defined(__ANDROID__)
struct JniFrame
{
    JNIEnv *env;
    ~JniFrame()
    {
        env->PopLocalFrame(nullptr);
    }
};

void check_jni(JNIEnv *env)
{
    if (!env->ExceptionCheck()) return;
    env->ExceptionClear();
    throw std::runtime_error(
        "Could not reach Cognito over HTTPS. Check your network and try again.");
}
#else
std::size_t receive(char *data, std::size_t size, std::size_t count, void *context)
{
    auto &body = *static_cast<std::string *>(context);
    const auto bytes = size * count;
    if (bytes > MAX_RESPONSE_SIZE - body.size()) return 0;
    body.append(data, bytes);
    return bytes;
}
#endif
} // namespace

CognitoHttpResponse cognito_https_post(const std::string &operation, const std::string &body)
{
#if defined(_WIN32)
    auto session = checked(WinHttpOpen(L"Snake/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!WinHttpSetTimeouts(session.get(), 5000, 5000, 10000, 10000))
        throw std::runtime_error("Unable to configure Cognito connection timeouts.");
    auto connection = checked(WinHttpConnect(session.get(), L"cognito-idp.us-west-2.amazonaws.com",
                                             INTERNET_DEFAULT_HTTPS_PORT, 0));
    auto request =
        checked(WinHttpOpenRequest(connection.get(), L"POST", L"/", nullptr, WINHTTP_NO_REFERER,
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy)))
        throw std::runtime_error("Unable to configure Cognito HTTPS.");
    const std::string target = "AWSCognitoIdentityProviderService." + operation;
    const std::wstring headers = L"Content-Type: application/x-amz-json-1.1\r\nX-Amz-Target: " +
                                 std::wstring(target.begin(), target.end()) + L"\r\n";
    if (!WinHttpSendRequest(request.get(), headers.c_str(), static_cast<DWORD>(headers.size()),
                            const_cast<char *>(body.data()), static_cast<DWORD>(body.size()),
                            static_cast<DWORD>(body.size()), 0) ||
        !WinHttpReceiveResponse(request.get(), nullptr))
        throw std::runtime_error(
            "Could not reach Cognito over HTTPS. Check your network and try again.");
    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
                             WINHTTP_NO_HEADER_INDEX))
        throw std::runtime_error("Unable to read Cognito response status.");
    std::string response;
    char buffer[4096];
    DWORD bytes = 0;
    do
    {
        if (!WinHttpReadData(request.get(), buffer, sizeof(buffer), &bytes))
            throw std::runtime_error("Unable to read Cognito response. Please try again.");
        if (bytes > MAX_RESPONSE_SIZE - response.size())
            throw std::runtime_error("Cognito response exceeded the size limit.");
        response.append(buffer, bytes);
    } while (bytes != 0);
    return {static_cast<int>(status), std::move(response)};
#elif defined(__ANDROID__)
    auto *env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
    if (env == nullptr) throw std::runtime_error("Android network bridge is unavailable.");
    if (env->PushLocalFrame(8) != JNI_OK)
    {
        if (env->ExceptionCheck()) env->ExceptionClear();
        throw std::runtime_error("Unable to initialize Android network bridge.");
    }
    const JniFrame frame{env};
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    check_jni(env);
    if (activity == nullptr) throw std::runtime_error("Android network bridge is unavailable.");
    jclass type = env->GetObjectClass(activity);
    check_jni(env);
    if (type == nullptr) throw std::runtime_error("Unable to initialize Android network bridge.");
    jmethodID method = env->GetMethodID(type, "postCognito", "([B[B)[B");
    check_jni(env);
    if (method == nullptr)
        throw std::runtime_error("Android network bridge method is unavailable.");
    jbyteArray operation_bytes = env->NewByteArray(static_cast<jsize>(operation.size()));
    check_jni(env);
    if (operation_bytes == nullptr) throw std::runtime_error("Unable to allocate network request.");
    jbyteArray body_bytes = env->NewByteArray(static_cast<jsize>(body.size()));
    check_jni(env);
    if (body_bytes == nullptr) throw std::runtime_error("Unable to allocate network request.");
    env->SetByteArrayRegion(operation_bytes, 0, static_cast<jsize>(operation.size()),
                            reinterpret_cast<const jbyte *>(operation.data()));
    check_jni(env);
    env->SetByteArrayRegion(body_bytes, 0, static_cast<jsize>(body.size()),
                            reinterpret_cast<const jbyte *>(body.data()));
    check_jni(env);
    auto response = static_cast<jbyteArray>(
        env->CallObjectMethod(activity, method, operation_bytes, body_bytes));
    check_jni(env);
    if (response == nullptr) throw std::runtime_error("Android network response is unavailable.");
    const auto length = env->GetArrayLength(response);
    check_jni(env);
    if (static_cast<std::size_t>(length) > MAX_RESPONSE_SIZE)
        throw std::runtime_error("Cognito response exceeded the size limit.");
    std::string text(static_cast<std::size_t>(length), '\0');
    env->GetByteArrayRegion(response, 0, length, reinterpret_cast<jbyte *>(text.data()));
    check_jni(env);
    // The Java bridge prefixes the response body with the three-digit HTTP status.
    if (text.size() < 3 || text[0] < '1' || text[0] > '5' || text[1] < '0' || text[1] > '9' ||
        text[2] < '0' || text[2] > '9')
        throw std::runtime_error("Invalid Android network response.");
    const int status = (text[0] - '0') * 100 + (text[1] - '0') * 10 + text[2] - '0';
    return {status, text.substr(3)};
#else
    static const auto initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (initialized != CURLE_OK) throw std::runtime_error("Unable to initialize HTTPS.");
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), curl_easy_cleanup);
    if (!handle) throw std::runtime_error("Unable to initialize Cognito connection.");
    const std::string target = "X-Amz-Target: AWSCognitoIdentityProviderService." + operation;
    curl_slist *list = curl_slist_append(nullptr, "Content-Type: application/x-amz-json-1.1");
    if (list == nullptr) throw std::runtime_error("Unable to allocate HTTPS headers.");
    auto *extended = curl_slist_append(list, target.c_str());
    if (extended == nullptr)
    {
        curl_slist_free_all(list);
        throw std::runtime_error("Unable to allocate HTTPS headers.");
    }
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(extended,
                                                                        curl_slist_free_all);
    std::string response;
    const auto set = [&](CURLoption option, auto value)
    {
        if (curl_easy_setopt(handle.get(), option, value) != CURLE_OK)
            throw std::runtime_error("Unable to configure Cognito HTTPS.");
    };
    set(CURLOPT_URL, "https://cognito-idp.us-west-2.amazonaws.com/");
    set(CURLOPT_HTTPHEADER, headers.get());
    set(CURLOPT_POSTFIELDS, body.c_str());
    set(CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    set(CURLOPT_CONNECTTIMEOUT, 5L);
    set(CURLOPT_TIMEOUT, 20L);
    set(CURLOPT_NOSIGNAL, 1L);
    set(CURLOPT_WRITEFUNCTION, receive);
    set(CURLOPT_WRITEDATA, &response);
    if (curl_easy_perform(handle.get()) != CURLE_OK)
        throw std::runtime_error(
            "Could not reach Cognito over HTTPS. Check your network and try again.");
    long status = 0;
    if (curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &status) != CURLE_OK)
        throw std::runtime_error("Unable to read Cognito response status.");
    return {static_cast<int>(status), std::move(response)};
#endif
}
