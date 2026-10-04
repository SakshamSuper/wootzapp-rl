#include <iostream>
#include <string>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

int main() {

    // --------------------------------------------------
    // 1. Open an HTTP session
    // --------------------------------------------------

    HINTERNET session = WinHttpOpen(
        L"WootzApp-CDP-Test",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        NULL,
        NULL,
        0
    );

    if (!session) {
        std::cout << "Could not open HTTP session.\n";
        return 1;
    }


    // --------------------------------------------------
    // 2. Connect to Chrome on port 9222
    // --------------------------------------------------

    HINTERNET connection = WinHttpConnect(
        session,
        L"localhost",
        9222,
        0
    );

    if (!connection) {
        std::cout << "Could not connect to Chrome on port 9222.\n";

        WinHttpCloseHandle(session);

        return 1;
    }


    // --------------------------------------------------
    // 3. Ask Chrome for its open pages
    // --------------------------------------------------

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"GET",
        L"/json",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!request) {
        std::cout << "Could not create HTTP request.\n";

        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }


    // --------------------------------------------------
    // 4. Send request and receive response
    // --------------------------------------------------

    BOOL sent = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!sent || !WinHttpReceiveResponse(request, NULL)) {

        std::cout << "Chrome did not respond.\n";

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }


    // --------------------------------------------------
    // 5. Read Chrome's JSON response
    // --------------------------------------------------

    std::string response;

    DWORD available = 0;

    while (
        WinHttpQueryDataAvailable(request, &available)
        && available > 0
    ) {

        char* buffer = new char[available + 1];

        DWORD bytesRead = 0;

        WinHttpReadData(
            request,
            buffer,
            available,
            &bytesRead
        );

        buffer[bytesRead] = '\0';

        response += buffer;

        delete[] buffer;
    }


    // --------------------------------------------------
    // 6. Find the MiniShop page
    // --------------------------------------------------

    size_t miniShopPosition =
        response.find("\"title\": \"MiniShop\"");

    if (miniShopPosition == std::string::npos) {

        std::cout << "MiniShop was not found.\n";

    } else {

        std::cout << "MiniShop found!\n";


        // --------------------------------------------------
        // 7. Find its WebSocket URL
        // --------------------------------------------------

        size_t websocketPosition =
            response.find(
                "\"webSocketDebuggerUrl\"",
                miniShopPosition
            );

        if (websocketPosition == std::string::npos) {

            std::cout
                << "WebSocket URL not found.\n";

        } else {

            // Find the colon after the field name.
            size_t colon =
                response.find(
                    ":",
                    websocketPosition
                );

            if (colon == std::string::npos) {

                std::cout
                    << "Could not find WebSocket URL value.\n";

            } else {

                // Find the opening quote of the URL.
                size_t start =
                    response.find(
                        "\"",
                        colon
                    );

                if (start == std::string::npos) {

                    std::cout
                        << "Could not find WebSocket URL start.\n";

                } else {

                    start++;

                    // Find the closing quote.
                    size_t end =
                        response.find(
                            "\"",
                            start
                        );

                    if (end == std::string::npos) {

                        std::cout
                            << "Could not find WebSocket URL end.\n";

                    } else {

                        std::string websocketUrl =
                            response.substr(
                                start,
                                end - start
                            );

                        std::cout
                            << "WebSocket URL:\n";

                        std::cout
                            << websocketUrl
                            << "\n";
                    }
                }
            }
        }
    }


    // --------------------------------------------------
    // 8. Clean up
    // --------------------------------------------------

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return 0;
}