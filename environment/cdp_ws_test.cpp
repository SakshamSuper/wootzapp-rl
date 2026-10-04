#include <iostream>
#include <string>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

int main() {

    // ==================================================
    // 1. Create WinHTTP session
    // ==================================================

    std::cout << "[1] Opening HTTP session...\n";

    HINTERNET session = WinHttpOpen(
        L"WootzApp-WebSocket-Test",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        NULL,
        NULL,
        0
    );

    if (!session) {
        std::cout << "ERROR: WinHttpOpen failed.\n";
        std::cout << "Error code: " << GetLastError() << "\n";
        return 1;
    }

    std::cout << "OK\n";


    // ==================================================
    // 2. Connect to Chrome CDP
    // ==================================================

    std::cout << "[2] Connecting to Chrome on port 9222...\n";

    HINTERNET connection = WinHttpConnect(
        session,
        L"localhost",
        9222,
        0
    );

    if (!connection) {
        std::cout << "ERROR: WinHttpConnect failed.\n";
        std::cout << "Error code: " << GetLastError() << "\n";

        WinHttpCloseHandle(session);

        return 1;
    }

    std::cout << "OK\n";


    // ==================================================
    // 3. MiniShop WebSocket path
    //
    // This was obtained from cdp_test.exe.
    // We will make this dynamic later.
    // ==================================================

    const wchar_t* websocketPath =
        L"/devtools/page/08AC87773BCEA02751D08B7FCBA04A87";

    std::cout << "[3] Using MiniShop WebSocket target...\n";


    // ==================================================
    // 4. Create HTTP request
    // ==================================================

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"GET",
        websocketPath,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!request) {
        std::cout << "ERROR: WinHttpOpenRequest failed.\n";
        std::cout << "Error code: " << GetLastError() << "\n";

        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }

    std::cout << "[4] HTTP request created.\n";


    // ==================================================
    // 5. Tell WinHTTP to upgrade HTTP to WebSocket
    // ==================================================

    BOOL upgradeOption = WinHttpSetOption(
        request,
        WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
        NULL,
        0
    );

    if (!upgradeOption) {
        std::cout << "ERROR: Could not enable WebSocket upgrade.\n";
        std::cout << "Error code: " << GetLastError() << "\n";

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }

    std::cout << "[5] WebSocket upgrade enabled.\n";


    // ==================================================
    // 6. Send the HTTP upgrade request
    // ==================================================

    BOOL sent = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!sent) {
        std::cout << "ERROR: WinHttpSendRequest failed.\n";
        std::cout << "Error code: " << GetLastError() << "\n";

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }

    std::cout << "[6] Upgrade request sent.\n";


    // ==================================================
    // 7. Receive Chrome's HTTP response
    // ==================================================

    BOOL received = WinHttpReceiveResponse(
        request,
        NULL
    );

    if (!received) {
        std::cout << "ERROR: WinHttpReceiveResponse failed.\n";
        std::cout << "Error code: " << GetLastError() << "\n";

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }

    std::cout << "[7] Chrome responded.\n";


    // ==================================================
    // 8. Read HTTP status code
    // ==================================================

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);

    BOOL headerResult = WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE |
        WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusSize,
        WINHTTP_NO_HEADER_INDEX
    );

    if (headerResult) {

        std::cout
            << "[8] HTTP status: "
            << statusCode
            << "\n";

    } else {

        std::cout
            << "[8] Could not read HTTP status.\n";

        std::cout
            << "Error code: "
            << GetLastError()
            << "\n";
    }


    // ==================================================
    // 9. Complete WebSocket upgrade
    // ==================================================

    std::cout << "[9] Completing WebSocket upgrade...\n";

    HINTERNET websocket =
        WinHttpWebSocketCompleteUpgrade(
            request,
            0
        );

    if (!websocket) {

        std::cout
            << "ERROR: WebSocket upgrade failed.\n";

        std::cout
            << "Error code: "
            << GetLastError()
            << "\n";

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }

    std::cout
        << "SUCCESS: WebSocket connection established!\n";


    // The original HTTP request is no longer needed.
    WinHttpCloseHandle(request);


    // ==================================================
    // 10. Create a Chrome DevTools Protocol command
    // ==================================================

    std::string message =
        R"({"id":1,"method":"Runtime.evaluate","params":{"expression":"document.title","returnByValue":true}})";


    std::cout
        << "[10] Sending CDP command...\n";


    // ==================================================
    // 11. Send CDP command through WebSocket
    // ==================================================

    DWORD sendResult =
        WinHttpWebSocketSend(
            websocket,
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)message.data(),
            static_cast<DWORD>(message.size())
        );

    if (sendResult != ERROR_SUCCESS) {

        std::cout
            << "ERROR: Could not send CDP command.\n";

        std::cout
            << "Error code: "
            << sendResult
            << "\n";

        WinHttpCloseHandle(websocket);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);

        return 1;
    }

    std::cout
        << "CDP command sent successfully.\n";


    // ==================================================
    // 12. Receive Chrome's response
    // ==================================================

    std::cout
        << "[12] Waiting for Chrome response...\n";

    char buffer[8192];

    DWORD bytesRead = 0;

    WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;


    DWORD receiveResult =
        WinHttpWebSocketReceive(
            websocket,
            buffer,
            sizeof(buffer) - 1,
            &bytesRead,
            &bufferType
        );

    if (receiveResult != ERROR_SUCCESS) {

        std::cout
            << "ERROR: Could not receive CDP response.\n";

        std::cout
            << "Error code: "
            << receiveResult
            << "\n";

    } else {

        buffer[bytesRead] = '\0';

        std::cout
            << "\n========== Chrome Response ==========\n";

        std::cout
            << buffer
            << "\n";

        std::cout
            << "=====================================\n";
    }


    // ==================================================
    // 13. Close WebSocket
    // ==================================================

    WinHttpWebSocketClose(
        websocket,
        WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
        NULL,
        0
    );


    // ==================================================
    // 14. Clean up
    // ==================================================

    WinHttpCloseHandle(websocket);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    std::cout
        << "\nDone.\n";

    return 0;
}