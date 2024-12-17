#include <iostream>
#include <string>
#include <memory>
#include <ctime>
#include <librdkafka/rdkafkacpp.h>
#include <avro/Encoder.hh>
#include <avro/Specific.hh>
#include "Message.hh"

class DeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
    void dr_cb(RdKafka::Message &message) {
        if (message.err())
            std::cerr << "Message delivery failed: " << message.errstr() << std::endl;
        else
            std::cout << "Message delivered to topic " << message.topic_name() << 
                     " [" << message.partition() << "] at offset <" << 
                     message.offset() << "> with content {" << message.payload() << "}. Status: " << message.status() << std::endl;
    }
};

int main() {
    std::string errstr;

    // Create Kafka configuration
    std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    std::unique_ptr<RdKafka::Conf> tconf(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));

    if (!conf) {
        std::cerr << "Failed to create config" << std::endl;
        return 1;
    }

    // Set configuration properties
    RdKafka::Conf::ConfResult conf_result;
    
    conf_result = conf->set("bootstrap.servers", "localhost:9092", errstr); // 172.18.0.2
    if (conf_result != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set bootstrap.servers: " << errstr << std::endl;
        return 1;
    }

    conf_result = conf->set("client.id", "kafka-producer-cpp", errstr);
    if (conf_result != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set client.id: " << errstr << std::endl;
        return 1;
    }

    // Set the delivery report callback
    DeliveryReportCb dr_cb;
    conf_result = conf->set("dr_cb", &dr_cb, errstr);
    if (conf_result != RdKafka::Conf::CONF_OK) {
        std::cerr << "Failed to set dr_cb: " << errstr << std::endl;
        return 1;
    }

    // Create producer
    std::unique_ptr<RdKafka::Producer> producer(RdKafka::Producer::create(conf.get(), errstr));
    if (!producer) {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        return 1;
    }

    // Create topic configuration
    std::string topic_name = "test_topic";
    std::unique_ptr<RdKafka::Topic> topic(
        RdKafka::Topic::create(producer.get(), topic_name, tconf.get(), errstr)
    );
    
    if (!topic) {
        std::cerr << "Failed to create topic: " << errstr << std::endl;
        return 1;
    }

    // Create test message
    IG::Message msg;
    msg.id = "123";
    msg.content = "Hello, Kafka!";
    msg.timestamp = static_cast<int64_t>(std::time(nullptr));

    // Serialize message
    std::ostringstream oss;
    avro::EncoderPtr encoder = avro::binaryEncoder();
    avro::OutputStreamPtr out = avro::ostreamOutputStream(oss);
    encoder->init(*out);
    avro::encode(*encoder, msg);
    out->flush();

    std::string serialized_str = oss.str();

    // Produce message
    RdKafka::ErrorCode err = producer->produce(
        topic.get(),
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<void*>(static_cast<const void*>(serialized_str.c_str())),
        serialized_str.size(),
        nullptr,    // Optional key
        nullptr     // Message opaque
    );

    if (err != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to produce message: " << RdKafka::err2str(err) << std::endl;
        return 1;
    } else {
        std::cout << "Message queued for delivery" << std::endl;
    }

    // Flush pending messages
    std::cout << "Flushing pending messages..." << std::endl;
    RdKafka::ErrorCode flush_err = producer->flush(10000);
    if (flush_err != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Failed to flush messages: " << RdKafka::err2str(flush_err) << std::endl;
        return 1;
    }

    std::cout << "Message delivered successfully" << std::endl;
    return 0;
}