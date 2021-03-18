@0x8c03f73b3ceae3de;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("Sim");

interface Simulator(Cmd) {
    loadFiles @0 (files :List(File)) -> (commands :Cmd);
}

interface Run {
    run @0 (vectors :List(Text)) -> (result :Result);
}

interface Tran {
    tran @0 (start :Float64, stop :Float64) -> (result :Result);
}

interface Result {
    read @0 (length :UInt64) -> (data :List(Vector));
    readTime @1 (seconds :Float64) -> (data :List(Vector));
    readAll @2 () -> (data :List(Vector));
}

struct Vector {
    name @0 :Text;
    data :union {
        real @1 :List(Float64);
        complex @2 :List(Complex);
    }
}

struct Complex {
    real @0 :Float64;
    imag @1 :Float64;
}

struct File {
    name @0 :Text;
    contents @1 :Data;
}

