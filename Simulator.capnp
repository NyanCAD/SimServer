@0x8c03f73b3ceae3de;

interface Simulator {
    loadFiles @0 (files :List(File)) -> (commands :List(Command));

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
        seek @3 (offset :UInt64) -> ();
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

    struct Command {
        union {
            run @0 :Run;
            tran @1 :Tran;
            #op @2 :Op;
            #ac @3 :Ac;
            # ...
        }
    }
}