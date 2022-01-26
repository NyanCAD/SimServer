#define NOMINMAX

#include <stdio.h>     // perror, printf
#include <stdlib.h>    // exit, atoi
#include <string.h>    // memset

#include "api/Simulator.capnp.h"
#include <kj/debug.h>
#include <kj/filesystem.h>
#include <kj/exception.h>
#include <kj/async-io.h>
#include <capnp/message.h>
#include <capnp/rpc-twoparty.h>

#include "../simserver.h"

#if defined WIN32
#include <winsock.h>
typedef int socklen_t;
#else
#define closesocket close
typedef int SOCKET;
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <signal.h>
#include <sys/wait.h>
#endif

# pragma comment(lib,"ws2_32.lib") //Winsock Library

class SimulatorImpl final : public Sim::Simulator<SimCommands>::Server
{
public:
    SimulatorImpl(const kj::Directory &dir) : dir(dir) {}

    kj::Promise<void> loadFiles(LoadFilesContext context) override
    {
        auto files = context.getParams().getFiles();
        for (Sim::File::Reader f : files)
        {
            kj::Path path = kj::Path::parse(f.getName());
            kj::Own<const kj::File> file = dir.openFile(path, kj::WriteMode::CREATE | kj::WriteMode::MODIFY | kj::WriteMode::CREATE_PARENT);
            file->truncate(0);
            file->write(0, f.getContents());
        }

        std::string name = files[0].getName();

        auto res = context.getResults();
        auto commands = kj::heap<SimCommandsImpl>(name);
        res.setCommands(kj::mv(commands));
        return kj::READY_NOW;
    }

    const kj::Directory &dir;
};

void handler(int sig) {
    int ret;
    wait(&ret);
}

int main(int argc, char const *argv[])
{
    #if defined WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2 ,2), &wsaData);
	if (iResult != 0) {
		printf("error at WSASturtup\n");
		return 0;
	}
    #else
    signal(SIGCHLD, handler);
    #endif

    SOCKET serverFd, clientFd;
    struct sockaddr_in server, client;
    socklen_t len;
    int port = 5923;
    char buffer[1024];
    if (argc == 2 && strcmp(argv[1], "--help")==0) {
        printf("%s [port]\n", argv[0]);
        return 0;
    } else if (argc == 2)
    {
        port = atoi(argv[1]);
    }
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        perror("Cannot create socket");
        exit(1);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    len = sizeof(server);
    if (bind(serverFd, (struct sockaddr *)&server, len) < 0)
    {
        perror("Cannot bind sokcet");
        exit(2);
    }
    if (listen(serverFd, 10) < 0)
    {
        perror("Listen error");
        exit(3);
    }
    printf("listening on port %d\n", port);
    while (1)
    {
        len = sizeof(client);
        printf("waiting for clients\n");
        if ((clientFd = accept(serverFd, (struct sockaddr *)&client, &len)) < 0)
        {
            perror("accept error");
            exit(4);
        }
        #if defined WIN32
        int pid = 0; // yolo no forking for you
        #else
        pid_t pid = fork();
        #endif
        if (pid < 0) {
            perror("fork error");
            exit(4);
        } else if (pid == 0) {
            char *client_ip = inet_ntoa(client.sin_addr);
            printf("Accepted new connection from a client %s:%d\n", client_ip, ntohs(client.sin_port));
            kj::AsyncIoContext ctx = kj::setupAsyncIo();
            auto stream = ctx.lowLevelProvider->wrapSocketFd(clientFd);
            auto network = capnp::TwoPartyVatNetwork(*stream, capnp::rpc::twoparty::Side::SERVER);
            kj::Own<kj::Filesystem> fs = kj::newDiskFilesystem();
            const kj::Directory &dir = fs->getCurrent();
            auto rpc = capnp::makeRpcServer(network, kj::heap<SimulatorImpl>(dir));
            network.onDisconnect().wait(ctx.waitScope);
            printf("Client disconnected: %s:%d\n", client_ip, ntohs(client.sin_port));
            #if !defined WIN32
            return 0;
            #endif
        }
    }
    closesocket(serverFd);
    #if defined WIN32
	WSACleanup();
    #endif
    return 0;
}
