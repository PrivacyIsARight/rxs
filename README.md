# rxs

A high-performance, lightweight [XMRig](https://github.com/xmrig/xmrig) fork focused exclusively on RandomX. By dropping support for other algorithms, rxs is able to achieve better performance and a smaller binary than XMRig. For the most part, rxs should be a drop in replacement for XMRig.

For KawPow, CryptoNight, GhostRider, or Windows support, use XMRig instead.

## Supported platforms

- **OS:** macOS, Linux, Android, BSDs
- **CPU:** x86/x64, ARMv7, ARMv8, RISC-V

## Building on macOS

Install CMake, libuv, hwloc, and OpenSSL (for example with Homebrew), then build a native Release binary:

```sh
brew install cmake libuv hwloc openssl@3
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build build --parallel
```

On macOS, the Release target is an app bundle at `build/rxs.app`. Double-click it to open Terminal for first-run setup. It asks once for a primary Monero address, saves the result at `~/Library/Application Support/rxs/config.json`, and starts mining from that per-user config.

To create native build artifacts (an `.app` and a `.zip`) for a Mac with the Homebrew dependencies above, run:

```sh
chmod +x scripts/build-macos.sh
scripts/build-macos.sh
```

Native CPU tuning is enabled by default. Pass `-DENABLE_MARCH_NATIVE=OFF` when producing a binary for a different Mac. Apple Silicon builds use the platform's hardened JIT API automatically.

The bundled configuration is tuned for maximum throughput: all CPU cores, no worker yielding, and maximum macOS thread QoS. It defaults to HashVault's TLS endpoint (`pool.hashvault.pro:443`), which currently advertises a 0% pool fee and hourly payouts. This does not guarantee a profit: earnings depend on hashrate, network difficulty, XMR price, power cost, and pool luck. RandomX is CPU-oriented; attempting to saturate an Apple GPU at the same time competes for unified-memory bandwidth and power, so rxs intentionally keeps the GPU idle when that produces more total hashes.
