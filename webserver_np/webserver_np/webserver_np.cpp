// NewsAppBackend.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/beast.hpp>
#include<queue>
#include<threads.h>
#include<map>
#include <mutex>
#include <condition_variable>
#include<rapidjson/document.h>
#include<rapidjson/writer.h>

#define GET 1
#define POST 2
#define PUT 3
#define DEL 4

using namespace boost::asio;
namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;
using namespace std;
std::mutex queue_mutex;
std::condition_variable queue_condition;

struct ReqDetails {
public:
    int Method;
    bool isinvalid = false;
    beast::http::request<beast::http::string_body> req_;
    tcp::socket socket_;
    //RequestHandler(tcp::socket socket) : socket_(std::move(socket)) {}

    ReqDetails(tcp::socket socket) : socket_(std::move(socket)) {}
};
queue<ReqDetails> que_RequestQueue;

std::string make_response()
{
    return "HTTP/1.1 200 OK\r\nContent-Length: 13\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nHello, world!";
}

struct ResOut {
    beast::http::status statuscode;
    string body;
};


std::string GetJsonValueAsString(rapidjson::Value& val)
{
    rapidjson::Document temp;
    temp.CopyFrom(val, temp.GetAllocator());

    rapidjson::StringBuffer tempSb;
    rapidjson::Writer<rapidjson::StringBuffer> tempWriter(tempSb);

    temp.Accept(tempWriter);
    return std::string(tempSb.GetString());
}

ResOut ProcessGetRequest(string path) {
    ResOut response;
    response.statuscode = beast::http::status::ok;

    if (path == "/api/getUsers") {
        cout << "Valid Req\n";
        rapidjson::Document doc;
        doc.SetObject();

        doc.AddMember("user1", "saran", doc.GetAllocator());

        string Resbody = GetJsonValueAsString(doc);

        response.body = Resbody;
    }

    return response;
}

void ProcessIncomingRequest()
{
    while (true)
    {
        Sleep(100);
        ;        while (que_RequestQueue.size() != 0)
        {
            std::cout << "Received request body:\n" << que_RequestQueue.front().req_.body() << std::endl;
            cout << "Method : " << que_RequestQueue.front().req_.method_string() << endl;
            ResOut Res_out;

            if (que_RequestQueue.front().req_.method_string() == "GET") {
                cout << "Processing GetRequest\n";
                Res_out = ProcessGetRequest(que_RequestQueue.front().req_.target());
            }
            else
            {
                beast::http::response<beast::http::string_body> response{ beast::http::status::bad_request,que_RequestQueue.front().req_.version() };
                response.set(beast::http::field::content_type, "text/plain");
                response.body() = "Bad Request";
                response.prepare_payload();
                beast::http::write(que_RequestQueue.front().socket_, response);
                que_RequestQueue.pop();

                continue;
            }

            cout << "Target : " << que_RequestQueue.front().req_.target() << endl;
            beast::http::response<beast::http::string_body> response{ Res_out.statuscode,que_RequestQueue.front().req_.version() };
            response.set(beast::http::field::content_type, "application/json");
            response.body() = Res_out.body;
            response.prepare_payload();
            beast::http::write(que_RequestQueue.front().socket_, response);
            que_RequestQueue.pop();
        }
    }
}

int main()
{
    try {
        io_context io_context;

        // Step 2: Create an acceptor to listen for incoming connections
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 5000)); // Listen on port 8080

        thread IncReq_Thread(ProcessIncomingRequest);

        while (true) {
            // Step 3: Create a socket and accept a connection
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            // Step 4: Read the HTTP request
            beast::flat_buffer buffer;
            beast::http::request<beast::http::string_body> request;
            beast::http::read(socket, buffer, request);

            // Step 5: Extract and print the request body


            ReqDetails req_details(std::move(socket));
            req_details.req_ = request;
            que_RequestQueue.push(std::move(req_details));

            // Step 6: Send the HTTP response
            /*beast::http::response<beast::http::string_body> response{beast::http::status::ok, req_details.req_.version()};
            response.set(beast::http::field::content_type, "text/plain");
            response.body() = "Hello, world!";
            response.prepare_payload();
            beast::http::write(req_details.socket_, response);
            */
        }

        if (IncReq_Thread.joinable())
            IncReq_Thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    cout << "close\n";
    return 0;
}

