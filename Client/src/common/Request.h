#ifndef REQUEST_H_
#define REQUEST_H_

#include <string>
#include <map>
#include <queue>

#include "PacketBuffer.h"

using namespace std;

const string kIPAttr = "IP";
const string kSubAction = "Subaction";

const string kStatus = "Status";
const string kBody = "Body";
const string kFilePrefix = "_File";

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
    protected:
        Action action = ACTION_INVALID;
        map<string, string> params;
    public:
        virtual ~Request();
        map<string, string>& getParams();
        void setParams(const map<string, string> &params);
        void putParam(string key, string value);
        string getParam(string key);
        void setAction(Action action);
        Action getAction() const;

        /* Socket */
        /**
         * Biến request thành dãy byte để chuẩn bị gửi.
         */
        void serialize(PacketBuffer &buf) const;
        /**
         * Chuyển dãy byte nhận được thành request.
         */
        void deserialize(PacketBuffer &buf);
        
        void toMailString(string &subject, string &body) const;
        /**
         * MIME Format
         */
        void parseFromMail(const string &mailHeaders, const string &mailBody, string &mailFrom, string &mailSubject);
       
        friend ostream& operator<<(ostream &os, Request &o);
};

class Response : public Request {
    public:
        vector<string> getFiles();
        void saveFiles();
        void deleteFiles();
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

string toString(Action);

Request parseRequestFromMail(const string &subject, const string &body);

#endif