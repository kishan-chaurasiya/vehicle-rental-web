#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int PORT = 8080;
const string PUBLIC_DIR = "public/";
const string DATA_DIR = "data/";

struct Vehicle {
    int id;
    string name, type, seats, fuel, transmission, price, image;
};

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == string::npos) return "";
    return s.substr(a, b-a+1);
}

string urlDecode(string s) {
    string out;
    for (size_t i=0; i<s.size(); ++i) {
        if (s[i] == '%' && i+2 < s.size()) {
            string h = s.substr(i+1,2);
            char c = (char)strtol(h.c_str(), nullptr, 16);
            out += c; i += 2;
        } else if (s[i] == '+') out += ' ';
        else out += s[i];
    }
    return out;
}

string formValue(const string& body, const string& key) {
    string target = key + "=";
    size_t p = body.find(target);
    if (p == string::npos) return "";
    p += target.size();
    size_t e = body.find('&', p);
    return urlDecode(body.substr(p, e == string::npos ? string::npos : e-p));
}

string jsonEscape(const string& s) {
    string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

vector<Vehicle> loadVehicles() {
    vector<Vehicle> v;
    ifstream f(DATA_DIR + "vehicles.csv");
    string line;
    getline(f, line);
    while (getline(f, line)) {
        stringstream ss(line);
        vector<string> x; string part;
        while (getline(ss, part, ',')) x.push_back(trim(part));
        if (x.size() >= 8)
            v.push_back({stoi(x[0]),x[1],x[2],x[3],x[4],x[5],x[6],x[7]});
    }
    return v;
}

string vehiclesJson() {
    auto v = loadVehicles();
    string j = "[";
    for (size_t i=0;i<v.size();++i) {
        if (i) j += ",";
        j += "{\"id\":"+to_string(v[i].id)+
             ",\"name\":\""+jsonEscape(v[i].name)+
             "\",\"type\":\""+jsonEscape(v[i].type)+
             "\",\"seats\":\""+jsonEscape(v[i].seats)+
             "\",\"fuel\":\""+jsonEscape(v[i].fuel)+
             "\",\"transmission\":\""+jsonEscape(v[i].transmission)+
             "\",\"price\":\""+jsonEscape(v[i].price)+
             "\",\"image\":\""+jsonEscape(v[i].image)+"\"}";
    }
    return j + "]";
}

string bookingsJson(const string& username) {
    ifstream f(DATA_DIR + "bookings.csv");
    string line, j = "[";
    getline(f,line);
    bool first=true;
    while (getline(f,line)) {
        stringstream ss(line); vector<string> x; string p;
        while(getline(ss,p,',')) x.push_back(trim(p));
        if(x.size() >= 7 && x[1] == username) {
            if(!first) j += ",";
            first=false;
            j += "{\"id\":\""+jsonEscape(x[0])+"\",\"username\":\""+jsonEscape(x[1])+
                 "\",\"vehicle\":\""+jsonEscape(x[2])+"\",\"pickup\":\""+jsonEscape(x[3])+
                 "\",\"dropoff\":\""+jsonEscape(x[4])+"\",\"days\":\""+jsonEscape(x[5])+
                 "\",\"total\":\""+jsonEscape(x[6])+"\"}";
        }
    }
    return j + "]";
}

string response(const string& body, const string& type="application/json", int code=200) {
    string status = code == 200 ? "200 OK" : code == 404 ? "404 Not Found" : "400 Bad Request";
    string h = "HTTP/1.1 " + status + "\r\nContent-Type: " + type +
               "\r\nContent-Length: " + to_string(body.size()) +
               "\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
    return h + body;
}

string mime(const string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".svg")) return "image/svg+xml";
    return "text/plain";
}

void sendAll(SOCKET s, const string& data) {
    size_t sent=0;
    while(sent<data.size()) {
        int n=send(s,data.c_str()+sent,(int)(data.size()-sent),0);
        if(n<=0) break;
        sent += n;
    }
}

void handleClient(SOCKET client) {
    char buffer[65536] = {};
    int received = recv(client, buffer, sizeof(buffer)-1, 0);
    if(received <= 0) { closesocket(client); return; }

    string req(buffer, received);
    size_t firstSpace=req.find(' ');
    size_t secondSpace=req.find(' ', firstSpace+1);
    if(firstSpace==string::npos || secondSpace==string::npos) {
        closesocket(client); return;
    }

    string method=req.substr(0,firstSpace);
    string target=req.substr(firstSpace+1,secondSpace-firstSpace-1);
    string body;
    size_t headerEnd=req.find("\r\n\r\n");
    if(headerEnd != string::npos) body=req.substr(headerEnd+4);

    if(method=="GET" && target=="/api/vehicles") {
        sendAll(client,response(vehiclesJson()));
    }
    else if(method=="POST" && target=="/api/register") {
        string user=formValue(body,"username"), pass=formValue(body,"password");
        if(user.empty() || pass.empty()) sendAll(client,response("{\"ok\":false,\"message\":\"Fill all fields\"}","application/json",400));
        else {
            ofstream f(DATA_DIR+"users.csv",ios::app);
            f << user << "," << pass << "\n";
            sendAll(client,response("{\"ok\":true,\"message\":\"Registration successful\"}"));
        }
    }
    else if(method=="POST" && target=="/api/login") {
        string user=formValue(body,"username"), pass=formValue(body,"password");
        ifstream f(DATA_DIR+"users.csv"); string line; bool ok=false;
        while(getline(f,line)) {
            stringstream ss(line); string u,p;
            getline(ss,u,','); getline(ss,p,',');
            if(u==user && p==pass) {ok=true; break;}
        }
        sendAll(client,response(string("{\"ok\":")+(ok?"true":"false")+
            ",\"message\":\""+(ok?"Login successful":"Invalid username or password")+"\"}"));
    }
    else if(method=="POST" && target=="/api/book") {
        string user=formValue(body,"username"), vehicle=formValue(body,"vehicle");
        string pickup=formValue(body,"pickup"), dropoff=formValue(body,"dropoff");
        string days=formValue(body,"days"), total=formValue(body,"total");
        if(user.empty() || vehicle.empty() || pickup.empty() || dropoff.empty()) {
            sendAll(client,response("{\"ok\":false,\"message\":\"Complete booking details\"}","application/json",400));
        } else {
            ofstream f(DATA_DIR+"bookings.csv",ios::app);
            long long id=time(nullptr);
            f << id << "," << user << "," << vehicle << "," << pickup << "," << dropoff
              << "," << days << "," << total << "\n";
            sendAll(client,response("{\"ok\":true,\"message\":\"Booking confirmed\"}"));
        }
    }
    else if(method=="GET" && target.rfind("/api/bookings?username=",0)==0) {
        string user=urlDecode(target.substr(21));
        sendAll(client,response(bookingsJson(user)));
    }
    else if(method=="GET") {
        if(target=="/") target="/index.html";
        string safe=target;
        size_t q=safe.find('?'); if(q!=string::npos) safe=safe.substr(0,q);
        if(safe.find("..")!=string::npos) {
            sendAll(client,response("Forbidden","text/plain",400));
        } else {
            if(safe.size()>0 && safe[0]=='/') safe=safe.substr(1);
            ifstream f(PUBLIC_DIR+safe,ios::binary);
            if(!f) {
                sendAll(client,response("404 - Page not found","text/plain",404));
            } else {
                string content((istreambuf_iterator<char>(f)),istreambuf_iterator<char>());
                sendAll(client,response(content,mime(safe)));
            }
        }
    }
    else {
        sendAll(client,response("Unsupported request","text/plain",400));
    }
    closesocket(client);
}

int main() {
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0) {
        cerr<<"WSAStartup failed\n"; return 1;
    }

    SOCKET server=socket(AF_INET,SOCK_STREAM,0);
    if(server==INVALID_SOCKET) { cerr<<"Socket creation failed\n"; WSACleanup(); return 1; }

    sockaddr_in addr{};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(PORT);

    if(bind(server,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR ||
       listen(server,10)==SOCKET_ERROR) {
        cerr<<"Could not start server. Is port "<<PORT<<" already in use?\n";
        closesocket(server); WSACleanup(); return 1;
    }

    cout<<"============================================\n";
    cout<<"       DriveEase Vehicle Rental Server\n";
    cout<<"============================================\n";
    cout<<"Server running at: http://localhost:"<<PORT<<"\n";
    cout<<"Press Ctrl+C to stop.\n\n";

    while(true) {
        SOCKET client=accept(server,nullptr,nullptr);
        if(client!=INVALID_SOCKET) handleClient(client);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
