import capnp
import Simulator_capnp as api
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

def loadFiles(sim, *names):
    data = []
    for name in names:
        with open(name) as f:
            data.append({
                "name": name,
                "contents": f.read()
            })
    return sim.loadFiles(data)

def map_complex(vec):
    return [complex(v.real, v.imag) for v in vec.complex]

def readAll(result):
    data = {}
    scale = None
    while True:
        res = result.read().wait()
        for vec in res.data:
            if vec.data.which() == 'real':
                data.setdefault(vec.name, []).extend(vec.data.real)
            elif vec.data.which() == 'complex':
                cdata = map_complex(vec.data)
                data.setdefault(vec.name, []).extend(cdata)
            else:
                raise TypeError(vec.data.which())

        if res.scale:
            scale = res.scale
        if not res.more: break
    df = pd.DataFrame(data)
    if scale:
        df[scale] = df[scale].abs()
        return df.set_index(scale)
    else:
        return df

if __name__ == "__main__":
    import sys
    sim = capnp.TwoPartyClient('localhost:5923').bootstrap().cast_as(api.Ngspice)
    res = loadFiles(sim, *sys.argv[1:])

    df = readAll(res.commands.op().result)
    print(df)

    df = readAll(res.commands.ac(api.AcType.dec, 10, 1, 1e6).result)
    print(df)
    fig, (ax1, ax2) = plt.subplots(nrows=2)
    df.abs().plot(ax=ax1, loglog=True)
    df.apply(np.angle).apply(np.rad2deg).plot(ax=ax2, logx=True)

    df = readAll(res.commands.tran(1e-6, 2e-3, 0, vectors=['@r1[i]']).result)
    print(df)
    df.plot()

    plt.show()