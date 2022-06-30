#include "HttpResponse.h"
#include "HttpParse.h"
#include "Header.h"
#include "HttpRequest.h"
#include "TimeStamp.h"
#include "HttpCallback.h"

#include <gflags/gflags.h>

#if 1

#include <chrono>


using namespace std::chrono;
using namespace std;
#endif

using namespace rapidjson;
DEFINE_string(serverName, "Pine", "server name");
std::map<std::string, std::string> httpContentTypes = {
        {"js",   "application/x-javascript"},
        {"css",  "text/css"},
        {"png",  "image/png"},
        {"jpg",  "image/jpg"},
        {"tar",  "application/tar"},
        {"zip",  "application/zip"},
        {"html", "text/html"},
        {"json", "application/json"},
        {"jpeg", "image/jpeg"},
        {"woff2","font/woff2"}
};


void HttpResponse::initHttpResponseHead() {
    // responseHead_->initHttpResponseHead(code);

    addHttpResponseHead("Server", FLAGS_serverName);
    addHttpResponseHead("Date", TimeStamp::getUTC());
}

HttpResponse::HttpResponse() : responseHead_(make_unique<ResponseHead>()), respData_(""),cookie_("") {}

void HttpResponse::sendResponseHeader(TcpClient* client){
    if(!respData_.empty()){
        responseHead_->responseHeader_ += respData_;
    }
    responseHead_->responseStatue_ += responseHead_->responseHeader_;
    LOG_INFO("send response http header:%s",responseHead_->responseStatue_.c_str());
    client->send(responseHead_->responseStatue_);
}

void HttpResponse::setCookie(const char* cookie,const char* path,int maxAge,bool httpOnly){
    cookie_ += ";";
    cookie_ +=  path;
    cookie_ +=  ";";
    cookie_ +=  to_string(maxAge);
    if(httpOnly){
        cookie_ += ";";
        cookie_ += "HttpOnly";
    }
}

void HttpResponse::SendFile(TcpClient *client, std::unique_ptr<HttpInfo> &httpInfo) {
    // �����û����ɽ������Ͳ�����response���󣬼����ȴ���Ϣ��
    if (!httpInfo->isParseFinish())return;
    auto &header = httpInfo->parse_->getHeader();
    if (header->method_ == "POST") {
        //POST��ִ�лص����������������ݣ�ע���߼�˳��
        auto cb = HttpCallback::getPostCB(header->requestURI.c_str());
        if(cb == nullptr){
            // todo ���ò����ڵĻص���������
            LOG_ERROR("%s not callback fun",header->requestURI.c_str());
        } else{
            cb(httpInfo.get());
        }
    } else if(header->method_ == "GET"){
        // GET�ȸ���200
        responseHead_->initHttpResponseHead(header->code_);
    }
    // ���������Ӧͷ�ļ�
    setHeaderResponse(header.get());

    // ����ͷ�ļ�
    sendResponseHeader(client);

    auto& reqFileInfo = header->reqFileInfo_;
    client->start = system_clock::now();
    char *buff = (char *) malloc(reqFileInfo->fileSize_);
    ::read(reqFileInfo->fileFd_, buff, reqFileInfo->fileSize_);
    // LOG_INFO("read buff:%d",n);
    string s(buff, reqFileInfo->fileSize_); // ���ܻ���ʧ�����ǲ���Ҫ�ж϶�������
    // auto end = system_clock::now();
    // auto duration = duration_cast<microseconds>(end - client->start);
    // cout <<  "��ȡ�ļ�������"
    // << double(duration.count()) * microseconds::period::num / microseconds::period::den
    // << "��" << endl;
    int n = client->send(std::move(s));
    free(buff);
    // �������ļ��ر��׽���
    close(reqFileInfo->fileFd_);

    // ����ʧ���Ѿ������closeCallback��
    if (n == -1 || header->kv_.find("Connection") == header->kv_.end() || header->kv_["Connection"].find("close") != string::npos) {
        client->CloseCallback(); // ���ǳ�������Ҫ�ر�
    }
}



void HttpResponse::addHttpResponseHead(string k, string v) {
    responseHead_->responseHeader_ += k + ":" + v;
    addHeaderEnd();
}

void HttpResponse::setResponseData(HTTP_STATUS_CODE code,std::string data) {
    // get post ����Ҫ���
    responseHead_->initHttpResponseHead(code);
    respData_ += data;
}

void HttpResponse::addHeaderEnd() {
    string s = "\r\n";
    responseHead_->responseHeader_ += s;
}

void HttpResponse::setConnection(Header *header) {
    // �ж��Ƿ����keep-alive
    // todo д��trimȥ������Ŀո�
    if (header->kv_.find("Connection") != header->kv_.end() && header->kv_["Connection"].find("close") == string::npos
        && header->code_ != HTTP_STATUS_CODE::NOT_FOUND) {
        addHttpResponseHead("Connection", "keep-alive");
    }
}

void HttpResponse::setContentLength(Header *header) {
    string key = "Content-Length";
    string value = "";
    // todo ��̫�Ծ�����Ҫ��һ�¡�
    if (header->method_ == "GET") {
        value = to_string(header->reqFileInfo_->fileSize_);
    } else if (header->method_ == "POST") {
        value = to_string(respData_.size());
    }
    addHttpResponseHead(key, value);
}

void HttpResponse::setContentType(Header *header) {
    string key = "Content-Type";
    string value = "";
    if (header->method_ == "GET") {
        // �Ҳ������ͣ����ϲ�ʹ�����ˣ����404.html��
        value = httpContentTypes[header->reqFileInfo_->fileType_];
    } else if (header->method_ == "POST") {
        value = httpContentTypes["json"];
    }
    value += "; charset=utf-8"; // ����utf-8����
    addHttpResponseHead(key, value);
}

void HttpResponse::setCookie(Header* header){
    if(cookie_.empty())return ;
    string k = "Set-Cookie";
    addHttpResponseHead(k,cookie_);
}

void HttpResponse::setHeaderResponse(Header *header) {
    // todo ��Ӧ״̬Ӧ���û��ṩ��ʼ��
   initHttpResponseHead();
    // �Ƿ�����
    setConnection(header);
    // �ظ��ĳ���
    setContentLength(header);
    //�ظ�������
    setContentType(header);
    // �Ƿ���Ҫ����cookie
    setCookie(header);
    // ����һ����β
    addHeaderEnd();
}

HttpResponse::~HttpResponse() {}