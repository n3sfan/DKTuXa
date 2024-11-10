#ifndef REQUEST_H_
#define REQUEST_H_

#include <string>
#include <map>
#include <queue>

#include "Request.h"

using namespace std;

const string kIPAttr = "IP";
const string kSubAction = "Subaction";

const string kStatus = "Status";
const string kBody = "Body";

enum Action {
    ACTION_INVALID,
    ACTION_SHUTDOWN,
    ACTION_APP,
    ACTION_SERVICES,
    ACTION_FILE,
    ACTION_SCREENSHOT,
    ACTION_WEBCAM,
    ACTION_KEYLOG
};

class Request {
    private:
        Action action = ACTION_INVALID;
        map<string, string> params;
    public:
        map<string, string>& getParams();
        void putParam(string key, string value);
        string getParam(string key);
        string serialize() const;
        void deserialize(const string &buf);
        void setAction(Action action);
        Action getAction() const;

        friend ostream& operator<<(ostream &os, Request &o);
};

class Response : public Request {
    public:
        friend ostream& operator<<(ostream &os, Response &o);
    // private:
    //     Action action;
    //     map<string, string> results;
    // public:
    //     map<string, string>& getResults() const;
    //     string serialize() const;
    //     void deserialize(const char *buf);
};

// TODO multi thread
extern queue<Response*> responsesQueue;

Action getAction(string name);

Request parseRequestFromMail(const string &subject, const string &body);

#endif