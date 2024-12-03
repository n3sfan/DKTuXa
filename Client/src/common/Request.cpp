#include "Request.h"

#include <utility>
#include <cstring>
#include <iostream>
#include <fstream>
#include <cstdio>

#include "Utils.h"

using namespace std;

queue<Response*> responsesQueue;

Action getAction(string name) {
    name = toLower(name);
    if (startsWith(name, "shutdown")) {
        return ACTION_SHUTDOWN;
    } else if (startsWith(name, "app")) {
        return ACTION_APP;
    } else if (startsWith(name, "service")) {
        return ACTION_SERVICES;
    } else if (startsWith(name, "file")) {
        return ACTION_FILE;
    } else if (startsWith(name, "screenshot")) {
        return ACTION_SCREENSHOT;
    } else if (startsWith(name, "webcam")) {
        return ACTION_WEBCAM;
    } else if (startsWith(name, "keylog")) {
        return ACTION_KEYLOG;
    } else if (startsWith(name, "broadcast")) {
        return ACTION_BROADCAST;
    } 
    return ACTION_INVALID;
}

Request::~Request() {
    
}

map<string, string>& Request::getParams() {
    return params;
}

void Request::setParams(const map<string, string> &params) {
    this->params = params;
}

void Request::putParam(string key, string value) {
    params[key] = value;
}

string Request::getParam(string key) {
    return params[key];
}

void Request::setAction(Action action) {
    this->action = action;
}

Action Request::getAction() const {
    return action;
}

/* Socket */
void Request::serialize(PacketBuffer &buffer) const {
    int len = 4 + 4; // total len + action number
    for (const pair<string, string>& pr : params) {
        len += 8;
        len += pr.first.size();
        len += pr.second.size();
    }
    buffer.setPacketSize(len);
    buffer.writeInt(len);
    int cur = 4;
    buffer.writeInt(action);
    cur += 4;

    // cout << "DEBUG send: " << len << "\n";
    
    for (const pair<string, string>& pr : params) {
        int l = pr.first.size();
        buffer.writeString(pr.first.c_str());
        buffer.writeString(pr.second.c_str());
        cur += l;
    }

    buffer.flush();
}

void Request::deserialize(PacketBuffer &buffer) {
    int requestSize;
    int cur = 0;
    requestSize = buffer.readInt();
    buffer.setPacketSize(requestSize);
    cur += 4;
    action = (Action)buffer.readInt();
    cur += 4;

    cout << "DEBUG deserialize: " << requestSize << "\n";

    string key;
    string value;
    while (buffer.getPacketPos() < buffer.getPacketSize()) {
        key.clear(); value.clear();
        key = buffer.readString();
        value = buffer.readString();
        params[key] = value;
    }
}

/* Mail */
void Request::toMailString(string &subject, string &body) const {
    // subject = toString(action);
    body = "";
    
    for (const pair<string, string>& pr : params) {
        if (pr.first == kPassWord){
            continue; // Chặn trường Password được gửi lại
        }
        if (pr.first[0] == '_') {
            continue; // Skip internal params
        }
        
        body += pr.first;
        body += ": ";
        body += pr.second;
        body += "\r\n";
    }
}

void Request::parseFromMail(const string &mailHeaders, const string &mailBody, string &mailFrom, string &mailSubject, string &mailMessageId) {
    bool body = false;
    int pos = mailHeaders.find("From: ");
    int pos2 = mailHeaders.find("<", pos + 6 + 1);
    int pos3 = mailHeaders.find(">", pos2 + 1);
    mailFrom = mailHeaders.substr(pos2 + 1, pos3 - pos2 - 1);

    pos = mailHeaders.find("Subject: ");
    pos2 = mailHeaders.find("\r\n", pos + 9 + 1);
    mailSubject = mailHeaders.substr(pos + 9, pos2 - pos - 1); 
    action = ::getAction(mailSubject);
    // cout << mailSubject << " mailSubject\n" << action << " action\n";

    pos = mailHeaders.find("Message-ID: ");
    pos2 = mailHeaders.find("\r\n", pos + 12 + 1);
    mailMessageId = mailHeaders.substr(pos + 12, pos2 - (pos + 12));

    // MIME Format
    vector<string> lines = split(mailBody, "\r\n");
    int n = (int)lines.size();
    for (int i = 0; i < n; ++i) {
        if (startsWith(lines[i], "Content-Type: text/plain")) {
            body = true;
        } else if (startsWith(lines[i], "--")) {
            body = false;
        } else if (body) {
            vector<string> entry = split(lines[i], ": ");
            if (entry.size() == 2) // in case empty break line
                putParam(entry[0], entry[1]); 
        }
    }
}

/* Files */
vector<string> Response::getFiles() {
    vector<string> res;
    for (const pair<string, string>& pr : params) {
        if (startsWith(pr.first, kFilePrefix)) {
            res.push_back(string("files/") + pr.first.substr(kFilePrefix.size()));
        }
    }
    return res;
}

void Response::saveFiles() {
    for (const pair<string, string>& pr : params) {
        if (startsWith(pr.first, kFilePrefix)) {
            ofstream fout((string("files/") + pr.first.substr(kFilePrefix.size())).c_str());
            fout.write(pr.second.c_str(), pr.second.size());
            fout.close();
        }
    }
}

void Response::deleteFiles() {
    for (const pair<string, string>& pr : params) {
        if (startsWith(pr.first, kFilePrefix)) {
            remove((string("files/") + pr.first.substr(kFilePrefix.size())).c_str());
        }
    }
}

/* I/O */
ostream& operator<<(ostream &os, Request &o) {
    os << "Action " << o.getAction() << "\n";
    for (const pair<string, string>& pr : o.getParams()) {
        if (pr.first == kPassWord){
            continue; // Mail phản hồi cấm cho xuất hiện mật khẩu trên terminal
        }
        os << pr.first << " " << pr.second << "\n";
    }
    return os;
}

ostream& operator<<(ostream &os, Response &o) {
    os << "Action " << o.getAction() << "\n";
    for (const pair<string, string>& pr : o.getParams()) {
        if (pr.first == kPassWord){
            continue; // Mail phản hồi cấm cho xuất hiện mật khẩu trên terminal
        }
        os << pr.first << " " << pr.second << "\n";
    }
    return os;
}

/* Helper */
Request parseRequestFromMail(const string &subject, const string &body) {
    Action action = getAction(subject);
    vector<string> lines = split(body, " ");
    Request request;
 
    for (string &line : lines) {
        vector<string> sp = split(line, ": ");
        if (sp.size() != 2) { // invalid
            continue;
        }
        request.putParam(trim(sp[0]), trim(sp[1]));
    }

    return request;
}