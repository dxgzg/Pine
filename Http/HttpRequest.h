#pragma once
#include <string>
#include <memory>
#include "HttpResponse.h"
#include "nocopyable.h"

class TcpClient;
class HttpInfo;

class HttpRequest : public nocopyable
{
public:
    HttpRequest() = default;
    ~HttpRequest();
 
    // 最开始执行的函数
    bool request(TcpClient*,std::string&,std::unique_ptr<HttpInfo>&);    
private:
};

