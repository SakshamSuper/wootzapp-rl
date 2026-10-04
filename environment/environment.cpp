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

    // Currently verified MiniShop Chrome target.
    const wchar_t* websocketPath =
        L"/devtools/page/08AC87773BCEA02751D08B7FCBA04A87";


    // --------------------------------------------------
    // Send a CDP command and receive the response
    // --------------------------------------------------

    bool sendCommand(
        const std::string& command,
        std::string& response
    ) {

        DWORD result = WinHttpWebSocketSend(
            websocket,
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)command.data(),
            static_cast<DWORD>(command.size())
        );

        if (result != ERROR_SUCCESS) {

            std::cout
                << "WebSocket send failed: "
                << result
                << "\n";

            return false;
        }


        char buffer[16384];

        DWORD bytesRead = 0;

        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;


        result = WinHttpWebSocketReceive(
            websocket,
            buffer,
            sizeof(buffer) - 1,
            &bytesRead,
            &bufferType
        );


        if (result != ERROR_SUCCESS) {

            std::cout
                << "WebSocket receive failed: "
                << result
                << "\n";

            return false;
        }


        buffer[bytesRead] = '\0';

        response = buffer;

        return true;
    }


    // --------------------------------------------------
    // Get current webpage observation
    // --------------------------------------------------

    bool observe() {

        std::string response;


        std::string command =
            "{\"id\":100,"
            "\"method\":\"Runtime.evaluate\","
            "\"params\":{"
            "\"expression\":\"document.body.innerText\","
            "\"returnByValue\":true"
            "}}";


        if (!sendCommand(command, response)) {
            return false;
        }


        std::cout
            << "\n========== OBSERVATION ==========\n";

        std::cout
            << response
            << "\n";

        std::cout
            << "=================================\n";


        return true;
    }


public:

    // ==================================================
    // RESET
    // ==================================================

    bool reset() {

        std::cout
            << "\n========== RESET ==========\n";


        // --------------------------------------------------
        // 1. Start WinHTTP
        // --------------------------------------------------

        session = WinHttpOpen(
            L"MiniShop-RL-Environment",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            NULL,
            NULL,
            0
        );


        if (!session) {

            std::cout
                << "WinHttpOpen failed: "
                << GetLastError()
                << "\n";

            return false;
        }


        // --------------------------------------------------
        // 2. Connect to Chrome
        // --------------------------------------------------

        connection = WinHttpConnect(
            session,
            L"localhost",
            9222,
            0
        );


        if (!connection) {

            std::cout
                << "WinHttpConnect failed: "
                << GetLastError()
                << "\n";

            return false;
        }


        // --------------------------------------------------
        // 3. Create WebSocket request
        // --------------------------------------------------

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

            std::cout
                << "WinHttpOpenRequest failed: "
                << GetLastError()
                << "\n";

            return false;
        }


        // --------------------------------------------------
        // 4. Enable WebSocket upgrade
        // --------------------------------------------------

        BOOL option = WinHttpSetOption(
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


        // --------------------------------------------------
        // 5. Send HTTP request
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


        if (!sent) {

            std::cout
                << "WinHttpSendRequest failed: "
                << GetLastError()
                << "\n";

            WinHttpCloseHandle(request);

            return false;
        }


        // --------------------------------------------------
        // 6. Receive Chrome response
        // --------------------------------------------------

        BOOL received = WinHttpReceiveResponse(
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


        // --------------------------------------------------
        // 7. Read HTTP status
        // --------------------------------------------------

        DWORD statusCode = 0;

        DWORD statusSize =
            sizeof(statusCode);


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


        // --------------------------------------------------
        // 8. Upgrade to WebSocket
        // --------------------------------------------------

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
        // 9. Initial observation
        // --------------------------------------------------

        return observe();
    }


    // ==================================================
    // STEP
    //
    // Supported:
    //
    // wait
    // click:0
    // click:1
    // click:2
    // ==================================================

    bool step(
        const std::string& action
    ) {

        std::cout
            << "\n========== STEP ==========\n";

        std::cout
            << "Action: "
            << action
            << "\n";


        if (!websocket) {

            std::cout
                << "Environment is not connected.\n";

            return false;
        }


        // ==================================================
        // WAIT
        // ==================================================

        if (action == "wait") {

            Sleep(500);

            return observe();
        }


        // ==================================================
        // CHECK CLICK ACTION
        // ==================================================

        if (
            action.rfind("click:", 0)
            != 0
        ) {

            std::cout
                << "Unknown action.\n";

            return false;
        }


        // --------------------------------------------------
        // Get button index
        // --------------------------------------------------

        int buttonIndex;


        try {

            buttonIndex =
                std::stoi(
                    action.substr(6)
                );

        }
        catch (...) {

            std::cout
                << "Invalid button index.\n";

            return false;
        }


        std::cout
            << "Button index: "
            << buttonIndex
            << "\n";


        // ==================================================
        // FIND BUTTON COORDINATES
        // ==================================================

        /*
         * JavaScript:
         *
         * 1. Find visible buttons.
         * 2. Select buttonIndex.
         * 3. Get its center coordinates.
         * 4. Return:
         *
         *      x,y,text
         *
         * We intentionally avoid JavaScript template
         * literals here to keep the C++ string simple.
         */

        std::string expression =
            "(()=>{"
            "const b=[...document.querySelectorAll('button')]"
            ".filter(x=>{"
            "const r=x.getBoundingClientRect();"
            "const s=getComputedStyle(x);"
            "return r.width>0&&"
            "r.height>0&&"
            "s.visibility!=='hidden'&&"
            "s.display!=='none'"
            "});"
            "const x=b[" +
            std::to_string(buttonIndex) +
            "];"
            "if(!x)return 'NOT_FOUND';"
            "const r=x.getBoundingClientRect();"
            "return "
            "String(r.left+r.width/2)+','+"
            "String(r.top+r.height/2)+','+"
            "x.innerText;"
            "})()";


        std::string command =
            "{\"id\":200,"
            "\"method\":\"Runtime.evaluate\","
            "\"params\":{"
            "\"expression\":\"";


        // Escape quotes for JSON.

        for (char c : expression) {

            if (c == '"') {
                command += "\\\"";
            }
            else if (c == '\\') {
                command += "\\\\";
            }
            else {
                command += c;
            }
        }


        command +=
            "\","
            "\"returnByValue\":true"
            "}}";


        std::string response;


        if (!sendCommand(
                command,
                response
            )) {

            return false;
        }


        std::cout
            << "Button information:\n";

        std::cout
            << response
            << "\n";


        // ==================================================
        // EXTRACT VALUE FROM CDP RESPONSE
        // ==================================================

        size_t valuePosition =
            response.find(
                "\"value\":\""
            );


        if (
            valuePosition
            == std::string::npos
        ) {

            std::cout
                << "Could not find button coordinates.\n";

            return false;
        }


        valuePosition += 9;


        size_t valueEnd =
            response.find(
                "\"",
                valuePosition
            );


        if (
            valueEnd
            == std::string::npos
        ) {

            std::cout
                << "Could not parse coordinates.\n";

            return false;
        }


        std::string value =
            response.substr(
                valuePosition,
                valueEnd - valuePosition
            );


        if (
            value
            == "NOT_FOUND"
        ) {

            std::cout
                << "Button does not exist.\n";

            return false;
        }


        // ==================================================
        // PARSE X
        // ==================================================

        size_t comma =
            value.find(",");


        if (
            comma
            == std::string::npos
        ) {

            std::cout
                << "Invalid coordinate format.\n";

            return false;
        }


        double x =
            std::stod(
                value.substr(
                    0,
                    comma
                )
            );


        // ==================================================
        // PARSE Y
        // ==================================================

        size_t secondComma =
            value.find(
                ",",
                comma + 1
            );


        if (
            secondComma
            == std::string::npos
        ) {

            std::cout
                << "Invalid coordinate format.\n";

            return false;
        }


        double y =
            std::stod(
                value.substr(
                    comma + 1,
                    secondComma - comma - 1
                )
            );


        std::cout
            << "Click coordinates: ("
            << x
            << ", "
            << y
            << ")\n";


        // ==================================================
        // REAL MOUSE PRESS
        // ==================================================

        std::string mouseDown =
            "{\"id\":201,"
            "\"method\":\"Input.dispatchMouseEvent\","
            "\"params\":{"
            "\"type\":\"mousePressed\","
            "\"x\":" +
            std::to_string(x) +
            ","
            "\"y\":" +
            std::to_string(y) +
            ","
            "\"button\":\"left\","
            "\"clickCount\":1"
            "}}";


        if (!sendCommand(
                mouseDown,
                response
            )) {

            std::cout
                << "Mouse press failed.\n";

            return false;
        }


        // ==================================================
        // REAL MOUSE RELEASE
        // ==================================================

        std::string mouseUp =
            "{\"id\":202,"
            "\"method\":\"Input.dispatchMouseEvent\","
            "\"params\":{"
            "\"type\":\"mouseReleased\","
            "\"x\":" +
            std::to_string(x) +
            ","
            "\"y\":" +
            std::to_string(y) +
            ","
            "\"button\":\"left\","
            "\"clickCount\":1"
            "}}";


        if (!sendCommand(
                mouseUp,
                response
            )) {

            std::cout
                << "Mouse release failed.\n";

            return false;
        }


        std::cout
            << "Real mouse click completed.\n";


        // Give MiniShop time to update.

        Sleep(300);


        // ==================================================
        // OBSERVE AFTER CLICK
        // ==================================================

        return observe();
    }


    // ==================================================
    // CLEANUP
    // ==================================================

    ~MiniShopEnvironment() {

        if (websocket) {

            WinHttpWebSocketClose(
                websocket,
                WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
                NULL,
                0
            );

            WinHttpCloseHandle(
                websocket
            );
        }


        if (connection) {

            WinHttpCloseHandle(
                connection
            );
        }


        if (session) {

            WinHttpCloseHandle(
                session
            );
        }
    }
};


// ======================================================
// MAIN
// ======================================================

int main() {

    MiniShopEnvironment env;


    // Start the environment.

    if (!env.reset()) {

        std::cout
            << "Environment reset failed.\n";

        return 1;
    }


    // Test a real click.

    env.step(
        "click:0"
    );


    return 0;
}