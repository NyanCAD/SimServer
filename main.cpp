#include <stdio.h>     // printf

#include <string> 

#include "api/Simulator.capnp.h"
#include <kj/debug.h>
#include <kj/filesystem.h>
#include <kj/exception.h>
#include <kj/async-io.h>
#include <capnp/message.h>
#include <capnp/rpc-twoparty.h>

#include "../simserver.h"

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/process/handles.hpp>

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

using boost::this_process::native_handle_type;

int runchild(native_handle_type fd) {
    kj::AsyncIoContext ctx = kj::setupAsyncIo();
    auto stream = ctx.lowLevelProvider->wrapSocketFd(fd);
    auto network = capnp::TwoPartyVatNetwork(*stream, capnp::rpc::twoparty::Side::SERVER);
    kj::Own<kj::Filesystem> fs = kj::newDiskFilesystem();
    const kj::Directory &dir = fs->getCurrent();
    auto rpc = capnp::makeRpcServer(network, kj::heap<SimulatorImpl>(dir));
    network.onDisconnect().wait(ctx.waitScope);
    printf("Client disconnected\n");
    return 0;
}

using namespace boost::asio;
using ip::tcp;

int main(int argc, char const *argv[])
{
    int port = 5923;
    if (argc == 2 && strcmp(argv[1], "--help")==0) {
        printf("%s [port]\n", argv[0]);
        return 0;
    } else if (argc == 3 && strcmp(argv[1], "--child")==0) {
        native_handle_type fd = std::stoi(argv[2]);
        return runchild(fd);
    } else if (argc == 2)
    {
        port = std::stoi(argv[1]);
    }
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v6(), port));
    while (1)
    {
        printf("waiting for clients\n");
        tcp::socket peersocket(io_service);
        acceptor.accept(peersocket);
        auto endpoint = peersocket.remote_endpoint();
        printf("Accepted new connection from a client %s:%d\n", endpoint.address(), endpoint.port());
        std::string fd = std::to_string(peersocket.native_handle());
        boost::process::spawn(argv[0], "--child", fd);
    }
    return 0;
}
