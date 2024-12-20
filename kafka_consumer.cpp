#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <librdkafka/rdkafkacpp.h>
#include <avro/Decoder.hh>
#include <avro/Specific.hh>
#include "Message.hh"

using json = nlohmann::json;

class KafkaConsumer {
public:
    KafkaConsumer(const std::string& brokers, const std::string& topic,
                 const std::string& group_id)
        : brokers_(brokers), topic_(topic), group_id_(group_id), running_(false) {

        init();
    }

    ~KafkaConsumer() {
        if (consumer_) {
            consumer_->close();
        }
    }

    // New method for consuming a single message
    bool consumeSingleMessage(int timeout_ms = 5000) {
        std::cout << "Waiting for message..." << std::endl;

        std::unique_ptr<RdKafka::Message> msg(
            consumer_->consume(timeout_ms));

        switch (msg->err()) {
            case RdKafka::ERR_NO_ERROR:
                deserializeAndProcess(msg->payload(), msg->len());
                return true;

            case RdKafka::ERR__TIMED_OUT:
                std::cout << "Timeout waiting for message" << std::endl;
                return false;

            case RdKafka::ERR__PARTITION_EOF:
                std::cout << "Reached end of partition" << std::endl;
                return false;

            default:
                std::cerr << "Consumer error: " << msg->errstr() << std::endl;
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
            throw std::runtime_error("Failed to create consumer: " + errstr);
        }

        // Subscribe to topic
        std::vector<std::string> topics = {topic_};
        RdKafka::ErrorCode err = consumer_->subscribe(topics);
        if (err != RdKafka::ERR_NO_ERROR) {
            throw std::runtime_error("Failed to subscribe to topic: " +
                                   std::string(RdKafka::err2str(err)));
        }

        std::cout << "Consumer initialized and subscribed to topic: " << topic_ << std::endl;
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

            // Convert to JSON and print
            json j = messageToJson(msg);
            std::cout << "Received message: " << j.dump(2) << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error deserializing message: " << e.what() << std::endl;
        }
    }

    std::string brokers_;
    std::string topic_;
    std::string group_id_;
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
    bool running_;
};

int main() {
    try {
        // Create consumer
        KafkaConsumer consumer("localhost:9092", "test_topic", "my-consumer-group");

        // Try to consume a single message with 5 second timeout
        bool received = consumer.consumeSingleMessage(5000);

        if (received) {
            std::cout << "Successfully consumed message. Shutting down." << std::endl;
        } else {
            std::cout << "No message received within timeout. Shutting down." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}