## Initial Setup
```bash
# For Ubuntu/Debian
sudo apt-get update
sudo apt-get install librdkafka-dev
sudo apt-get install libavro-dev
sudo apt-get install docker-compose
sudo apt-get install cmake g++
sudo apt-get install libboost-dev libboost-filesystem-dev
sudo apt-get install libboost-iostreams-dev libboost-program-options-dev
sudo apt-get install libboost-iostreams-dev libboost-program-options-dev libboost-system-dev
sudo apt-get install nlohmann-json3-dev
sudo apt-get install libwebsocketpp-dev libboost-all-dev
```

## Avro
See [Avro CPP Page](https://avro.apache.org/docs/1.12.0/api/cpp/html/) to install avro.
1. Get the latst avro distro (e.g. `wget https://dlcdn.apache.org/avro/avro-1.12.0/cpp/avro-cpp-1.12.0.tar.gz` - present in root)
2. Expand the tarball into a directory (`tar -xvf avro-cpp-1.12.0.tar.gz`).
2. Change to lang/c++ subdirectory.
3. Type `./build.sh test`. This builds Avro C++ and runs tests on it.
4. Type `./build.sh install`. This installs Avro C++ under /usr/local on your system.

## Run the app
```bash
# Pull and run Kafka using Docker Compose
$ docker-compose up -d

# Build it
$ ./build.sh

# Run it
$ ./kafka_producer
$ ./kafka_consumer
```

## Errors
1. `libavrocpp.so.1.12.0: cannot open shared object file: No such file or directory`
```bash
$ ./kafka_producer 
./kafka_producer: error while loading shared libraries: libavrocpp.so.1.12.0: cannot open shared object file: No such file or directory

$ sudo bash -c 'echo "/usr/local/lib" > /etc/ld.so.conf.d/avro.conf'
$ sudo ldconfig
$ ldconfig -p | grep avro
        libavrocpp.so.1.12.0 (libc6,x86-64) => /usr/local/lib/libavrocpp.so.1.12.0
        libavrocpp.so (libc6,x86-64) => /usr/local/lib/libavrocpp.so
        libavro.so.23 (libc6,x86-64) => /lib/x86_64-linux-gnu/libavro.so.23
        libavro.so (libc6,x86-64) => /lib/x86_64-linux-gnu/libavro.so
```

## Sample Output
```
$ ./kafka_producer 
KP: Generated ID: ID-1472
KP: Message queued for delivery
KP: Flushing pending messages...
KP: Message delivered to topic test_topic [0] at offset <17> with content {0x5c372b467118}. Status: 2
KP: Message delivered successfully


$ ./kafka_consumer 
WS: WebSocket server listening on port 9002
KC: Consumer initialized and subscribed to topic: test_topic
KC: Waiting for a single message...

WS: Broadcasted message: {"content":"Hello, Kafka!","id":"ID-1472","timestamp":1734675172}

KC: Message forwarded to WebSocket clients
KC: Message processed. Consumer will now exit.
KC: Successfully consumed one message. Exiting.
```