#include <iostream>
#include <string>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

class MiniShopEnvironment {

private:

    HINTERNET session = nullptr;
    HINTERNET connection = nullptr;
    HINTERNET websocket = nullptr;

public:

    // --------------------------------------------------
    // RESET
    // --------------------------------------------------

    bool reset() {

        std::cout << "\n=== RESET ===\n";


        // 1. Open WinHTTP session
        session = WinHttpOpen(
            L"MiniShop-RL-Environment",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            NULL,
            NULL,
            0
        );

        if (!session) {
            std::cout << "WinHttpOpen failed: "
                      << GetLastError() << "\n";
            return false;
        }


        // 2. Connect to Chrome
        connection = WinHttpConnect(
            session,
            L"localhost",
            9222,
            0
        );

        if (!connection) {
            std::cout << "WinHttpConnect failed: "
                      << GetLastError() << "\n";
            return false;
        }


        // 3. Current MiniShop target
        //
        // We already verified this target using
        // cdp_test.exe.
        //
        const wchar_t* path =
            L"/devtools/page/08AC87773BCEA02751D08B7FCBA04A87";


        // 4. Create request
        HINTERNET request =
            WinHttpOpenRequest(
                connection,
                L"GET",
                path,
                NULL,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0
            );

        if (!request) {

            std::cout
                << "WinHttpOpenRequest failed: "
                << GetLastError()
                << "\n";

            return false;
        }


        // 5. Enable WebSocket upgrade
        BOOL option =
            WinHttpSetOption(
                request,
                WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
                NULL,
                0
            );

        if (!option) {

            std::cout
                << "WebSocket option failed: "
                << GetLastError()
                << "\n";

            WinHttpCloseHandle(request);

            return false;
        }


        // 6. Send request
        BOOL sent =
            WinHttpSendRequest(
                request,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                WINHTTP_NO_REQUEST_DATA,
                0,
                0,
                0
            );

        if (!sent) {

            std::cout
                << "WinHttpSendRequest failed: "
                << GetLastError()
                << "\n";

            WinHttpCloseHandle(request);

            return false;
        }


        // 7. Receive response
        BOOL received =
            WinHttpReceiveResponse(
                request,
                NULL
            );

        if (!received) {

            std::cout
                << "WinHttpReceiveResponse failed: "
                << GetLastError()
                << "\n";

            WinHttpCloseHandle(request);

            return false;
        }


        // 8. Show HTTP status
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);

        WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE |
            WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX
        );

        std::cout
            << "HTTP status: "
            << statusCode
            << "\n";


        // 9. Complete WebSocket upgrade
        websocket =
            WinHttpWebSocketCompleteUpgrade(
                request,
                0
            );

        WinHttpCloseHandle(request);


        if (!websocket) {

            std::cout
                << "WebSocket upgrade failed: "
                << GetLastError()
                << "\n";

            return false;
        }


        std::cout
            << "WebSocket connected!\n";


        // --------------------------------------------------
        // FIRST OBSERVATION
        // --------------------------------------------------

        std::string command =
            R"({"id":1,"method":"Runtime.evaluate","params":{"expression":"document.title","returnByValue":true}})";


        DWORD result =
            WinHttpWebSocketSend(
                websocket,
                WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                (PVOID)command.data(),
                static_cast<DWORD>(command.size())
            );


        if (result != ERROR_SUCCESS) {

            std::cout
                << "Send failed: "
                << result
                << "\n";

            return false;
        }


        char buffer[8192];

        DWORD bytesRead = 0;

        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;


        result =
            WinHttpWebSocketReceive(
                websocket,
                buffer,
                sizeof(buffer) - 1,
                &bytesRead,
                &bufferType
            );


        if (result != ERROR_SUCCESS) {

            std::cout
                << "Receive failed: "
                << result
                << "\n";

            return false;
        }


        buffer[bytesRead] = '\0';


        std::cout
            << "Observation:\n";

        std::cout
            << buffer
            << "\n";


        return true;
    }


    // --------------------------------------------------
    // STEP
    // --------------------------------------------------

    bool step(const std::string& action) {

        std::cout
            << "\n=== STEP ===\n";

        std::cout
            << "Action: "
            << action
            << "\n";


        if (!websocket) {

            std::cout
                << "Environment is not connected.\n";

            return false;
        }


        // For now we only observe the page.
        //
        // Real click(i) will be added next.

        std::string command =
            R"({"id":2,"method":"Runtime.evaluate","params":{"expression":"document.title","returnByValue":true}})";


        DWORD result =
            WinHttpWebSocketSend(
                websocket,
                WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                (PVOID)command.data(),
                static_cast<DWORD>(command.size())
            );


        if (result != ERROR_SUCCESS) {

            std::cout
                << "Step send failed: "
                << result
                << "\n";

            return false;
        }


        char buffer[8192];

        DWORD bytesRead = 0;

        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;


        result =
            WinHttpWebSocketReceive(
                websocket,
                buffer,
                sizeof(buffer) - 1,
                &bytesRead,
                &bufferType
            );


        if (result != ERROR_SUCCESS) {

            std::cout
                << "Step receive failed: "
                << result
                << "\n";

            return false;
        }


        buffer[bytesRead] = '\0';


        std::cout
            << "New observation:\n";

        std::cout
            << buffer
            << "\n";


        return true;
    }


    // --------------------------------------------------
    // CLEANUP
    // --------------------------------------------------

    ~MiniShopEnvironment() {

        if (websocket) {

            WinHttpWebSocketClose(
                websocket,
                WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
                NULL,
                0
            );

            WinHttpCloseHandle(websocket);
        }


        if (connection) {
            WinHttpCloseHandle(connection);
        }


        if (session) {
            WinHttpCloseHandle(session);
        }
    }
};


// ======================================================
// MAIN
// ======================================================

int main() {

    MiniShopEnvironment env;


    if (!env.reset()) {

        std::cout
            << "Reset failed.\n";

        return 1;
    }


    env.step("wait");


    return 0;
}