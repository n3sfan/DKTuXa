#ifndef REQUEST_H_
#define REQUEST_H_

#include <string>
#include <map>
#include <queue>

#include "PacketBuffer.h"
#include "SHA256.h"

const std::string kIPAttr = "IP";
const std::string kSubAction = "Sub Action";
const std::string kUseHtml = "_UseHtml";

const std::string kStatus = "Status";
const std::string kBody = "Body";
const std::string kFilePrefix = "_File";

const std::string kPassWord = "Password"; // Trường Password
const std::string kPcName = "PC Name"; // Trường nhận tên PC Name

const std::string kEmail = "quangminhcantho43@gmail.com";
const std::string kAppPass = "sigc xldk cuzd bjhr";


enum Action {
    ACTION_INVALID,
    ACTION_SHUTDOWN,
    ACTION_APP,
    ACTION_SERVICES,
    ACTION_FILE,
    ACTION_SCREENSHOT,
    ACTION_WEBCAM,
    ACTION_KEYLOG,
    ACTION_BROADCAST
};

class Request {
    protected:
        Action action = ACTION_INVALID;
        std::map<std::string, std::string> params;
    public:
        virtual ~Request();
        std::map<std::string, std::string>& getParams();
        void setParams(const std::map<std::string, std::string> &params);
        void putParam(std::string key, std::string value);
        std::string getParam(std::string key);
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
        
        void toMailString(std::string &subject, std::string &body) const;
        /**
         * MIME Format
         */
        void parseFromMail(const std::string &mailHeaders, const std::string &mailBody, std::string &mailFrom, std::string &mailSubject, std::string &mailMessageId);
       
        friend std::ostream& operator<<(std::ostream &os, Request &o);
};

class Response : public Request {
    public:
        std::vector<std::string> getFiles();
        void saveFiles();
        void deleteFiles();
        friend std::ostream& operator<<(std::ostream &os, Response &o);
    // private:
    //     Action action;
    //     std::map<string, std::string> results;
    // public:
    //     std::map<string, std::string>& getResults() const;
    //     std::string serialize() const;
    //     void deserialize(const char *buf);
};

// TODO multi thread
extern std::queue<Response*> responsesQueue;

Action getAction(std::string name);

std::string toString(Action);

Request parseRequestFromMail(const std::string &subject, const std::string &body);

#endif