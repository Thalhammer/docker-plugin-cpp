# Docker plugin C++
Provides a simple to use and dependency free library for building docker plugins in C++
similar to [go-plugins-helpers](https://github.com/docker/go-plugins-helpers). The library
provides a simple unix domain socket server that understands http 1.1 for interfacing with
docker, as well as types and serialization for its apis.

The entire library is designed single threaded, using select for handling io. There is no
interest in implementing support for things like multiple threads or epoll, since the limiting
factor for speed is docker and the single threaded design makes it a lot easier to test/verify.
You are free to use threads for your implementation or call different plugin instances from multiple
threads. The library uses picojson, which is included in the source tree and llhttp, which is pulled using
CMake FetchContent and built alongside the library

Plugin support:
- [X] Volume
- [ ] Authorization
- [ ] Network (planned)
- [ ] IPAM (planned)
- [ ] Graph
- [ ] Secrets (docker status unclear, but interesting)

A simple example for a volume driver can be found in `sample_volume`. It effectively reimplements dockers local volumes.

Contributions, Bug reports and improvements/feature requests are welcome. Pull requests are even better though ;)
