#include "UDP.h"

using namespace std;

bool isBroadcast = false;
std::map<std::string, std::string> pcNameIPMap;

/**
 * Protocol 2 - UDP version.
 */
int sendUDP(Request &request, Response &response) {
    SOCKET UdpSocket = INVALID_SOCKET;
    struct sockaddr_in destAddr;
    int iResult;

    LAN lan;
    lan.getWiFiIPAndSubnet();
    lan.calculateBroadcastIP();

    // Tính địa chỉ Broadcast IP trong file class LAN (ip, subnet, broadcast IP)
    std::string broadcastIP = lan.getbroadcastIP();
    printf("DEBUG: Broadcast IP is %s\n", broadcastIP.c_str());

    // Tạo socket UDP
    UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

    // Cấu hình địa chỉ đích
    ZeroMemory(&destAddr, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(atoi(DEFAULT_PORT2)); // Chuyển đổi port sang dạng network byte order
    destAddr.sin_addr.s_addr = inet_addr(broadcastIP.c_str());

    // Serialize dữ liệu request vào PacketBuffer
    PacketBuffer buffer(UdpSocket, false, &destAddr);
    printf("DEBUG: Sending %zu bytes to broadcast on port %s...\n", buffer.getBuffer().size(), DEFAULT_PORT2);
    request.serialize(buffer);
    printf("DEBUG: Data sent via broadcast. Waiting for responses...\n");

    //Nhận phản hồi từ các máy
    fd_set readfds;
    timeval timeout;
    timeout.tv_sec = 5; // Chờ phản hồi trong 5 giây
    timeout.tv_usec = 0;
    bool hasResponses = false;

    do {
        FD_ZERO(&readfds);
        FD_SET(UdpSocket, &readfds);

        // Chờ dữ liệu
        int activity = select(0, &readfds, NULL, NULL, &timeout);
        if (activity == SOCKET_ERROR) {
            printf("select failed with error: %d\n", WSAGetLastError());
            break;
        }

        if (activity == 0) { // Timeout hết thời gian chờ
            printf("DEBUG: Timeout reached, no more responses.\n");
            break;
        }

        if (FD_ISSET(UdpSocket, &readfds)) {
            sockaddr_in senderAddr;
            int senderAddrSize = sizeof(senderAddr);

            // Xử lý phản hồi
            printf("DEBUG: Received response from %s\n", inet_ntoa(senderAddr.sin_addr));
            PacketBuffer responseBuffer(UdpSocket, true, &senderAddr);
            response.deserialize(responseBuffer);

            string PCInfo = response.getParam(kBody);
            processAndStore(PCInfo, pcNameIPMap); // Lưu thông tin phản hồi
            hasResponses = true;
        }
    } while (true);

    if (!hasResponses) {
        printf("DEBUG: No responses received.\n");
    }

    // Dọn dẹp tài nguyên
    closesocket(UdpSocket);
    WSACleanup();

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
    string host = request.getParam(kIPAttr);
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

    CSMTPClient SMTPClient([](const std::string& s){ cout << s << "\n"; return; });  
    SMTPClient.SetCertificateFile("curl-ca-bundle.crt");
    SMTPClient.InitSession("smtp.gmail.com:465", "quangminhcantho43@gmail.com", kAppPass,
			CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);

    // Receive mail
    while (true) {
        CIMAPClient IMAPClient([](const std::string& s){ cout << s << "\n"; return; });  
        IMAPClient.SetCertificateFile("curl-ca-bundle.crt");
        IMAPClient.InitSession("imap.gmail.com:993", "quangminhcantho43@gmail.com", kAppPass,
                CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);
       
        str = "";
        bool ok = IMAPClient.Search(str, CIMAPClient::SearchOption::UNSEEN);
        cout << str << "\n";

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
            if (request.getAction() == ACTION_BROADCAST)
            {
                if (sendUDP(request, response) != 0) {
                    // TODO ERROR
                    continue;
                }
                string valueBodyBroadcast = "Lists of PC name and IP address: \n";
                for (const auto &x : pcNameIPMap){
                    valueBodyBroadcast += x.first;
                    valueBodyBroadcast += " - ";
                    valueBodyBroadcast += x.second;
                    valueBodyBroadcast += "\n";
                }
                response.putParam(kBody, valueBodyBroadcast);
            }
            else 
            {
                if (sendTCP(request, response) != 0) {
                    // TODO ERROR
                    continue;
                }
            }
            // Send mail response
            string mailStr; 
            response.toMailString(mailSubject, mailStr);
            
            // response.saveFiles();

            SMTPClient.SendMIME(mailFrom, {"Subject: " + mailSubject}, mailStr, response.getFiles());
            
            // response.deleteFiles();'

            cout << "---------\nResponse: " << response << "\n-------------\n";
            // Add to queue
            // responsesQueue.push();
        }   

        IMAPClient.CleanupSession();

        // TODO SLOW
        this_thread::sleep_for(4s);
    }    

    SMTPClient.CleanupSession();
}

