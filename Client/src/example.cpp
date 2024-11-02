#include <iostream>
#include <string>
#include <thread>
#include <windows.h>

#include "IMAPClient.h"
#include "SMTPClient.h"

using namespace std;

const string kAppPass = "";

int test() {
    // SMTP, gui mail
    CSMTPClient SMTPClient([](const std::string& s){ cout << s << "\n"; return; });  
    SMTPClient.SetCertificateFile("curl-ca-bundle.crt");
    SMTPClient.InitSession("smtp.gmail.com:465", "quangminhcantho43@gmail.com", kAppPass,
			CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);
    SMTPClient.SendMIME("<quangminhcantho43@gmail.com>", "<ltthinh23@clc.fitus.edu.vn>", {"Subject: example sending a MIME-formatted message"}, "Test sending\r\nTest", {});
    SMTPClient.CleanupSession();

    // IMAP, nhan mail
    CIMAPClient IMAPClient([](const std::string& s){ cout << s << "\n"; return; });  
    IMAPClient.SetCertificateFile("curl-ca-bundle.crt");
    IMAPClient.InitSession("imap.gmail.com:993", "quangminhcantho43@gmail.com", kAppPass,
			CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);


    string str;
    IMAPClient.ListSubFolders(str);
    cout << str << '\n';

    str = "";
    bool b = IMAPClient.Search(str, CIMAPClient::SearchOption::SEEN);
    cout << b << '\n';
    cout << str << '\n';

    str = ""; 
    cout << "\nGetString\n";
    b = IMAPClient.GetString("4420", str);
    cout << b << '\n';
    cout << str << '\n';

    string cmd;
    while (1) {
        getline(cin, cmd);
        str = "";
        b = IMAPClient.Fetch(cmd, str);


        cout << b << " res\n";
        cout << str << '\n';
    }

    IMAPClient.CleanupSession();
    return 0;
}