#include "Request.h"

#include <utility>
#include <cstring>
#include <iostream>
#include <fstream>
#include <cstdio>

#include "Utils.h"

using namespace std;

queue<Response*> responsesQueue;

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
string Request::serialize() const {
    int len = 4 + sizeof(Action); // total len + action number
    for (const pair<string, string>& pr : params) {
        len += 8;
        len += pr.first.size();
        len += pr.second.size();
    }
    string res;
    res.resize(len);
    memcpy(res.data(), &len, 4);
    int cur = 4;
    memcpy(res.data() + cur, (char*)&action, sizeof(Action));
    cur += 4;

    // cout << "DEBUG send: " << len << "\n";
    
    for (const pair<string, string>& pr : params) {
        int l = pr.first.size();
        memcpy(res.data() + cur, &l, 4);
        cur += 4; 
        strncpy(res.data() + cur, pr.first.c_str(), l);
        cur += l;
        
        l = pr.second.size();
        memcpy(res.data() + cur, &l, 4);
        cur += 4;
        strncpy(res.data() + cur, pr.second.c_str(), l);
        cur += l;
    }

    return res;
}

void Request::deserialize(const string &s) {
    const char *buf = s.c_str();
    int len;
    int cur = 0;
    memcpy((char*)&len, buf + cur, 4);
    cur += 4;
    memcpy((char*)&action, buf + cur, sizeof(Action));
    cur += 4;

    cout << "DEBUG deserialize: " << len << "\n";
    if (len != s.size()) {
        cout << "Debug: Fatal: Request/Response is corrupted!\n";
    }

    string key;
    string value;
    while (cur < s.size()) {
        key.clear(); value.clear();
   
        memcpy((char*)&len, buf + cur, 4);
        cur += 4; 
        key.resize(len);
        memcpy(key.data(), buf + cur, len);
        cur += len;

        memcpy((char*)&len, buf + cur, 4);
        cur += 4;
        value.resize(len);
        memcpy(value.data(), buf + cur, len); 
        cur += len;

        params[key] = value;
    }
}

/* Mail */
void Request::toMailString(string &subject, string &body) const {
    // subject = toString(action);
    body = "";
    
    for (const pair<string, string>& pr : params) {
        body += pr.first;
        body += ": ";
        body += pr.second;
        body += "\r\n";
    }
}

void Request::parseFromMail(const string &mailHeaders, const string &mailBody, string &mailFrom, string &mailSubject) {
    bool body = false;
    int pos = mailHeaders.find("From: ");
    int pos2 = mailHeaders.find("<", pos + 6 + 1);
    int pos3 = mailHeaders.find(">", pos2 + 1);
    mailFrom = mailHeaders.substr(pos2 + 1, pos3 - pos2 - 1);

    pos = mailHeaders.find("Subject: ");
    pos2 = mailHeaders.find("\r\n", pos + 9 + 1);
    mailSubject = mailHeaders.substr(pos + 9, pos2 - pos - 1); 
    action = ::getAction(mailSubject);

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
        os << pr.first << " " << pr.second << "\n";
    }
    return os;
}

ostream& operator<<(ostream &os, Response &o) {
    os << "Action " << o.getAction() << "\n";
    for (const pair<string, string>& pr : o.getParams()) {
        os << pr.first << " " << pr.second << "\n";
    }
    return os;
}

/* Helper */
Action getAction(string name) {
    name = toLower(name);
    if (name == "shutdown") {
        return ACTION_SHUTDOWN;
    } else if (name.find("app") != string::npos) {
        return ACTION_APP;
    } else if (name.find("services") != string::npos) {
        return ACTION_SERVICES;
    } else if (name.find("file") != string::npos) {
        return ACTION_FILE;
    } else if (name.find("screenshot") != string::npos) {
        return ACTION_SCREENSHOT;
    } else if (name.find("webcam") != string::npos) {
        return ACTION_WEBCAM;
    } else if (name.find("keylog") != string::npos) {
        return ACTION_KEYLOG;
    }
    return ACTION_INVALID;
}

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