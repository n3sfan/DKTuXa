#include "UDP.h"

using namespace std;

bool isBroadcast = false;
std::map<std::string, std::string> pcNameIPMap;

/**
 * Protocol 2 - UDP version.
 */
int sendUDP(Request &request, Response &response) {
    struct sockaddr_in destAddr;
    int iResult;

    LAN lan;
    lan.getWiFiIPAndSubnet();
    lan.calculateBroadcastIP();

    // Tính địa chỉ Broadcast IP trong file class LAN (ip, subnet, broadcast IP)
    std::string broadcastIP = lan.getbroadcastIP();
    printf("DEBUG: Broadcast IP is %s\n", broadcastIP.c_str());

    // Cấu hình địa chỉ đích
    ZeroMemory(&destAddr, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(atoi(DEFAULT_PORT2)); // Chuyển đổi port sang dạng network byte order
    destAddr.sin_addr.s_addr = inet_addr(broadcastIP.c_str());

    // Tạo socket UDP
    SOCKET UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (UdpSocket == INVALID_SOCKET) {
        printf("Socket creation failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Cho phép socket gửi broadcast
    BOOL broadcastPermission = TRUE;
    iResult = setsockopt(UdpSocket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcastPermission, sizeof(broadcastPermission));
    if (iResult == SOCKET_ERROR) {
        printf("setsockopt for broadcast failed with error: %d\n", WSAGetLastError());
        closesocket(UdpSocket);
        WSACleanup();
        return 1;
    }
    
    // Serialize dữ liệu request vào PacketBuffer
    printf("DEBUG: Data sent via broadcast. Waiting for responses...\n");
    // for (int tries = 0; tries < 10; ++tries) {
        PacketBuffer buffer(UdpSocket, false, &destAddr);
        // printf("DEBUG: Sending %zu bytes to broadcast on port %s...\n", buffer.getBuffer().size(), DEFAULT_PORT2);
        request.serialize(buffer);

        //Nhận phản hồi từ các máy
        fd_set readfds;
        timeval timeout;
        timeout.tv_sec = 2; // Chờ phản hồi trong 15 giây
        timeout.tv_usec = 0;
        bool hasResponses = false;

        do {
            FD_ZERO(&readfds);
            FD_SET(UdpSocket, &readfds);

            // Chờ dữ liệu
            int activity = select(0, &readfds, NULL, NULL, &timeout);
            if (activity == SOCKET_ERROR) {
                printf("select failed with error: %d\n", WSAGetLastError());
                // break;
                return 1;
            }

            if (activity == 0) { // Timeout hết thời gian chờ
                printf("DEBUG: Timeout reached, no more responses.\n");
                // break;
                return 0;
            }

            if (FD_ISSET(UdpSocket, &readfds)) {
                sockaddr_in senderAddr;
                int senderAddrSize = sizeof(senderAddr);

                // Xử lý phản hồi
                PacketBuffer responseBuffer(UdpSocket, true, &senderAddr);
                if (responseBuffer.getBuffer().size() > 0) {
                    response.deserialize(responseBuffer);

                    string PCInfo = response.getParam(kBody);
                    processAndStore(PCInfo, pcNameIPMap); // Lưu thông tin phản hồi
                    hasResponses = true;
                }
                printf("DEBUG: Received response from %s\n", inet_ntoa(senderAddr.sin_addr));
            }
        } while (true);
       
        if (!hasResponses) {
            printf("DEBUG: No responses received.\n");
        }
        
        std::this_thread::sleep_for(1s);
    // }
    // Dọn dẹp tài nguyên
    closesocket(UdpSocket);

    return 0;
}

int sendTCP(Request &request, Response &response) {
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    unique_ptr<char[]> recvbuf(new char[8192]());
    const int recvbuflen = 8192;
    int iResult;
    
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    string host;
    if (!request.getParam(kIPAttr).empty()){
        host = request.getParam(kIPAttr);
    }
    else if (!request.getParam(kPcName).empty()){
        for (auto pi: pcNameIPMap){
            if (pi.first == request.getParam(kPcName))
                host = pi.second;
        }
    }

    iResult = getaddrinfo(host.c_str(), DEFAULT_PORT2, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    // std::cout << "Socket addr: " << ConnectSocket << "\n";
    PacketBuffer buffer(ConnectSocket, false);
    request.serialize(buffer);

    // printf("Bytes Sent: %ld / %d\n", len, total);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    buffer = PacketBuffer(ConnectSocket, true);
    response.getParams().clear();
    response.deserialize(buffer);

    closesocket(ConnectSocket);
    
    // Download Manager - New sockets
    Downloader downloader;

    for (const std::pair<std::string, std::string> &pr : response.getParams()) {
        if (!startsWith(pr.first, kFilePrefix))
            continue;

        std::string file = pr.first.substr(kFilePrefix.size());
        cout << "DEBUG: Downloading file " << file << "\n";
        downloader.downloadFile(host, DEFAULT_PORT2, file);
        cout << "DEBUG: Downloaded file " << file << "\n";
    }

    downloader.joinThread();

    return 0;
}

void listenToInboxUDP() {
    string str;

    // Receive mail
    while (true) {
        CIMAPClient IMAPClient([](const std::string& s){ cout << s << "\n"; return; });  
        IMAPClient.SetCertificateFile("curl-ca-bundle.crt");
        IMAPClient.InitSession("imap.gmail.com:993", kEmail, kAppPass,
                CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);
       
        str = "";
        bool ok = IMAPClient.Search(str, CIMAPClient::SearchOption::UNSEEN);
        cout << str << "\n";

        bool broadcasted = false;

        vector<string> mailIds = split(str.substr(string("* SEARCH ").size()), " ");
        for (string &mailId : mailIds) {
            mailId = trim(mailId);
            if (mailId.size() == 0)
                continue;
            
            string mailHeaders = "", mailBody = "";
            ok = IMAPClient.GetString(mailId, mailHeaders);
            ok = IMAPClient.GetString(mailId, mailBody, true); 
            if (!ok) {
                // TODO NOTIFY
                continue;
            } 

            // cout << mailHeaders << " headers5\n";
            // cout << mailBody << " body\n";

            string mailFrom, mailSubject, mailMessageId;
            Request request;
            request.parseFromMail(mailHeaders, mailBody, mailFrom, mailSubject, mailMessageId);

            if (!isPassWordValid(request.getParam(kPassWord))){
                // TODO NOTIFY
                cout << "Invalid password.\n";
                continue;
            }
            if (request.getAction() == ACTION_INVALID) {
                // TODO NOTIFY
                continue;
            }
            cout << "Processing request, action " << request.getAction() << "\n";
            Response response;

            //Nếu là broadcast thì gửi bằng UDP
            if (request.getAction() != ACTION_BROADCAST) {
                continue;
            }
                
            if (sendUDP(request, response) != 0) {
                // TODO ERROR
                continue;
            }
            
            // string valueBodyBroadcast = "Lists of PC name and IP address: \n";
            // for (const auto &x : pcNameIPMap){
            //     valueBodyBroadcast += x.first;
            //     valueBodyBroadcast += " - ";
            //     valueBodyBroadcast += x.second;
            //     valueBodyBroadcast += "\n";
            // }
            // response.putParam(kBody, valueBodyBroadcast);

            // List PC Name and IP Address
            std::string valueBodyBroadcast = pcNameIPHTML(pcNameIPMap);
            response.putParam(kUseHtml, "true");
            response.putParam(kBody, valueBodyBroadcast);

            // Send mail response
            string mailStr; 
            response.toMailString(mailSubject, mailStr);
            
            // response.saveFiles();

            CSMTPClient SMTPClient([](const std::string& s){ cout << s << "\n"; return; });  
            SMTPClient.SetCertificateFile("curl-ca-bundle.crt");
            SMTPClient.InitSession("smtp.gmail.com:465", kEmail, kAppPass,
			                    CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);
            SMTPClient.SendMIME(mailFrom, {"References: " + mailMessageId, "In-Reply-To: " + mailMessageId, "Subject: Re: " + mailSubject}, mailStr, response.getFiles(), toLower(response.getParam(kUseHtml)) == "true");
            SMTPClient.CleanupSession();
            
            // response.deleteFiles();'

            // cout << "---------\nResponse: " << response << "\n-------------\n";
            broadcasted = true;
            // Add to queue
            // responsesQueue.push();
        }   

        IMAPClient.CleanupSession();

        if (broadcasted)
            break;

        // TODO SLOW
        this_thread::sleep_for(4s);
    }    
}

std::string pcNameIPHTML(std::map<std::string, std::string> pcNameIPMap){
    std::string pcNameIPHTML = "<!DOCTYPE html><html><head>";
    pcNameIPHTML += "<style>"
                    "body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #e9ecef; color: #333; }"
                    "div.table-container { background-color: #e9ecef; width: 70%; margin: 20px auto; padding: 20px; border-radius: 16px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); }"
                    "table { width: 80%; border-collapse: collapse; font-family: Arial, sans-serif; margin: auto;}"
                    "th, td { border: 1px solid #ddd; padding: 8px; text-align: center; font-size: 14px; }"
                    "th { background-color: #6c757d; color: white; font-size: 16px; }"
                    "td { background-color: #fff;}"
                    "tr:nth-child(even) { background-color: #f8f9fa; }"
                    "tr:hover { background-color: #e2e6ea; }"
                    "h1 { text-align: center; color: #495057; }"
                    "p { text-align: center; color: #666; margin-top: 20px; }"
                    "</style></head><body>";


    pcNameIPHTML += "<h1 style=\"text-align: center; color: #333;\">PC Name and IP Address</h1>";
    pcNameIPHTML += "<div class=\"table-container\">";
    pcNameIPHTML += "<table>";
    pcNameIPHTML += "<thead><tr><th>STT</th><th>PC Name</th><th>IP Address</th></tr></thead><tbody>";

    int index = 1;
    for (const auto& pc : pcNameIPMap) {
        pcNameIPHTML += "<tr>";
        pcNameIPHTML += "<td>" + std::to_string(index++) + "</td>";
        pcNameIPHTML += "<td>" + pc.first + "</td>";
        pcNameIPHTML += "<td>" + pc.second + "</td>";
        pcNameIPHTML += "</tr>";
    }

    pcNameIPHTML += "</tbody></table></div>";
    pcNameIPHTML += "<p style=\"text-align: center; margin-top: 20px; color: #666; font-size: 14px;\">Total PCs: " + std::to_string(pcNameIPMap.size()) + "</p>";
    pcNameIPHTML += "</body></html>";

    return pcNameIPHTML;
}