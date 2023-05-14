#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

using namespace std;
#define SERVER_PORT 8000
#define BUFFER_SIZE 1024
#define THREAD_NUM 200
const long long CacheSize = 1024 * 1024 * 10; // 10MB

// 缓存类
class Cache
{
public:
    void put(string key, string value)
    {
        if (cache_.size() >= CacheSize)
        {
            cache_.erase(cache_.begin());
        }
        cache_[key] = value;
    }

    bool get(string key, string &value)
    {
        auto it = cache_.find(key);
        if (it != cache_.end())
        {
            value = it->second;
            return true;
        }
        return false;
    }

private:
    map<string, string> cache_;
    /*
    std::map 是一个基于红黑树的关联容器，它将键按照一定的顺序进行排序
    std::map 的插入、删除和查找操作的时间复杂度都是 O(log n)
    */
};
// 定义全局缓存池
Cache cache_pool;


// HTTP 请求结构体
struct HttpRequest
{
    string method;
    string path;
    string version;
    vector<pair<string, string>> headers; // 请求头中的键值对如 >Host: 127.0.0.1:8000
    string file_content;
};

// HTTP 响应结构体
struct HttpResponse
{
    string version;
    int status;
    string reason;
    vector<pair<string, string>> headers;
    string file_content;
};

// HTTP 状态码和原因短语映射表
static const map<int, string> status_reasons = {
    {200, "OK"},
    {404, "Not Found"},
    {500, "Internal Server Error"}};

// 读取文件内容
string read_file(const string &path)
{
    ifstream file(path, ios::binary);
    if (!file)
    {
        return "";
    }
    stringstream file_content;
    file_content << file.rdbuf();
    return file_content.str();
}

// 解析 HTTP 请求
HttpRequest parse_request(const string &request_str)
{
    HttpRequest request;
    istringstream iss(request_str);
    getline(iss, request.method, ' ');
    getline(iss, request.path, ' ');
    getline(iss, request.version); // 默认读到换行符则停止
    string line;                   // 读取剩余行 >Host: 127.0.0.1:8000
    while (getline(iss, line) && !line.empty())
    {
        size_t pos = line.find(':');
        if (pos != string::npos)
        {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            request.headers.emplace_back(key, value);
        }
    }
    return request;
}

// 生成 HTTP 响应
HttpResponse make_response(int status, const string &file_content = "") // file_content有默认值
{
    HttpResponse response;
    response.version = "HTTP/1.0";
    response.status = status;
    response.reason = status_reasons.at(status);
    response.headers.emplace_back("Content-Length", to_string(file_content.size()));
    // response.headers.emplace_back("Content-Type", "text/html");
    response.file_content = file_content;
    return response;
}

// 处理 HTTP 请求
void handle_request(int client_sock)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n;
    while (true)
    {
        n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
        {
             cerr << "recv Bad File Descriptor\n" << endl;
            close(client_sock);
            return;
        }
        if (strstr(buffer, "\r\n\r\n") != NULL)
            break;
    }
    string request_str(buffer, n); // 类型转换
    HttpRequest request = parse_request(request_str);

    // check if method is GET and path starts with "/"
    if (request.method != "GET" || request.path[0] != '/')
    {
        stringstream response;
        response << "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        send(client_sock, response.str().c_str(), response.str().length(), 0);
        close(client_sock);
        return;
    }
    if (request.path.find("..") != string::npos)
    {
        stringstream response;
        response << "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        send(client_sock, response.str().c_str(), response.str().length(), 0);
        close(client_sock);
        return;
    }
    if (request.path.compare("/") == 0)
    {
        request.path += "hello.html";
    }
    if (request.path.find_last_of(".") == string::npos) // 请求资源为一个目录
    {
        stringstream response;
        response << "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        send(client_sock, response.str().c_str(), response.str().length(), 0);
        close(client_sock);
        return;
    }

    string path = "." + request.path; // 加上服务器根目录
    string file_content;
    // 从缓存池中获取数据
    cache_pool.get(path, file_content);
    // 如果缓存池中不存在该资源，则从磁盘上读取
    if (file_content.empty())
    {
        file_content = read_file(path);

    }
    HttpResponse res;
    if (file_content.empty())
    {
        res = make_response(404);
    }
    else
    {
        res = make_response(200, file_content);
    }
    // 将读取到的数据添加到缓存池中
    cache_pool.put(path, file_content);

    // 输出相应信息
    string response = res.version + " " + to_string(res.status) + " " + res.reason + "\r\n"; // 换行不可少
    for (const auto &header : res.headers)
    {
        response += header.first + ": " + header.second + "\r\n";
    }
    response += "\r\n" + res.file_content; // 空行不可少
    int bytes_sent = 0;
    while (bytes_sent < response.size())
    {
        int ret = send(client_sock, response.c_str() + bytes_sent, response.size() - bytes_sent, 0);
        if (ret < 0)
        {
            continue;// 发送失败，进行重试
        }
        bytes_sent += ret;
    }
    close(client_sock);
    return;
}

int main()
{
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0)
    {
        cerr << "Failed to create socket" << endl;
        return 1;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Failed to bind socket" << endl;
        return 1;
    }
    if (listen(server_sock, THREAD_NUM) < 0)
    {
        cerr << "Failed to listen on socket" << endl;
        return 1;
    }
    cout << "Server started on port " << SERVER_PORT << endl;
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0)
        {
            cerr << "Failed to accept client connection" << endl;
            continue;
        }
        cout << "Accepted client connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;
        thread t(handle_request, client_sock);
        t.detach();
    }
    close(server_sock);
    return 0;
}
