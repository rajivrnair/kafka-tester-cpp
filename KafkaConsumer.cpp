#include <iostream>
#include <thread>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <librdkafka/rdkafkacpp.h>
#include <avro/Decoder.hh>
#include <avro/Specific.hh>
#include "Message.hh"
#include "WebSocketServer.hpp"


using json = nlohmann::json;

class KafkaConsumer {
public:
    KafkaConsumer(const std::string& brokers, const std::string& topic,
                 const std::string& group_id, WebSocketServer& ws_server)
        : brokers_(brokers), topic_(topic), group_id_(group_id),
          ws_server_(ws_server) {
        init();
    }

    ~KafkaConsumer() {
        if (consumer_) {
            consumer_->close();
        }
    }

    // Consume a single message and return true if successful
    bool consumeOne(int timeout_ms = 5000) {
        std::cout << "KC: Waiting for a single message..." << std::endl;

        std::unique_ptr<RdKafka::Message> msg(
            consumer_->consume(timeout_ms));

        switch (msg->err()) {
            case RdKafka::ERR_NO_ERROR:
                deserializeAndProcess(msg->payload(), msg->len());
                std::cout << "KC: Message processed. Consumer will now exit." << std::endl;
                return true;

            case RdKafka::ERR__TIMED_OUT:
                std::cout << "KC: Timeout waiting for message" << std::endl;
                return false;

            case RdKafka::ERR__PARTITION_EOF:
                std::cout << "KC: Reached end of partition" << std::endl;
                return false;

            default:
                std::cerr << "KC: Consumer error: " << msg->errstr() << std::endl;
                return false;
        }
    }

private:
    void init() {
        std::string errstr;

        // Create configuration
        std::unique_ptr<RdKafka::Conf> conf(
            RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));

        if (!conf) {
            throw std::runtime_error("Failed to create configuration");
        }

        // Set configuration properties
        conf->set("bootstrap.servers", brokers_, errstr);
        conf->set("group.id", group_id_, errstr);
        conf->set("auto.offset.reset", "earliest", errstr);

        // Create consumer
        consumer_.reset(RdKafka::KafkaConsumer::create(conf.get(), errstr));
        if (!consumer_) {
            throw std::runtime_error("KC: Failed to create consumer: " + errstr);
        }

        // Subscribe to topic
        std::vector<std::string> topics = {topic_};
        RdKafka::ErrorCode err = consumer_->subscribe(topics);
        if (err != RdKafka::ERR_NO_ERROR) {
            throw std::runtime_error("KC: Failed to subscribe to topic: " +
                                   std::string(RdKafka::err2str(err)));
        }

        std::cout << "KC: Consumer initialized and subscribed to topic: " << topic_ << std::endl;
    }

    json messageToJson(const IG::Message& msg) {
        return json{
            {"id", msg.id},
            {"content", msg.content},
            {"timestamp", msg.timestamp}
        };
    }

    void deserializeAndProcess(const void* payload, size_t len) {
        try {
            // Create input stream from message payload
            auto input = avro::memoryInputStream(
                static_cast<const uint8_t*>(payload), len);
            avro::DecoderPtr decoder = avro::binaryDecoder();
            decoder->init(*input);

            // Deserialize into Message object
            IG::Message msg;
            avro::decode(*decoder, msg);

            // Convert to JSON
            json j = messageToJson(msg);

            // Forward to WebSocket server
            if (ws_server_.broadcast(j.dump())) {
                std::cout << "KC: Message forwarded to WebSocket clients" << std::endl;
            } else {
                std::cerr << "KC: Failed to forward message to WebSocket clients" << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "KC: Error processing message: " << e.what() << std::endl;
        }
    }

    std::string brokers_;
    std::string topic_;
    std::string group_id_;
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
    WebSocketServer& ws_server_;
};

int main() {
    try {
        // Create and start WebSocket server in a separate thread
        WebSocketServer ws_server;
        std::thread ws_thread([&ws_server]() {
            ws_server.run();
        });

        // Wait a moment for WebSocket server to start
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Create consumer
        KafkaConsumer consumer("localhost:9092", "test_topic", "my-consumer-group", ws_server);

        // Consume a single message with 5 second timeout
        bool received = consumer.consumeOne(5000);

        // Stop WebSocket server
        ws_server.stop();
        ws_thread.join();

        if (received) {
            std::cout << "KC: Successfully consumed one message. Exiting." << std::endl;
            return 0;
        } else {
            std::cout << "KC: No message received within timeout. Exiting." << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}