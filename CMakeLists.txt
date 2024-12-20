cmake_minimum_required(VERSION 3.10)
project(kafka_avro_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_library(RDKAFKA_LIBRARY rdkafka++)
find_library(AVROCPP_LIBRARY avrocpp)

# Find avrogencpp executable
find_program(AVROGENCPP avrogencpp REQUIRED)
if(NOT AVROGENCPP)
    message(FATAL_ERROR "avrogencpp not found!")
endif()

# Set Avro schema file and generated output
set(AVRO_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/message.avsc")
set(AVRO_HPP "${CMAKE_CURRENT_BINARY_DIR}/Message.hh")

# Add custom command to generate C++ from Avro schema
add_custom_command(
    OUTPUT ${AVRO_HPP}
    COMMAND ${AVROGENCPP} -i ${AVRO_SCHEMA} -o ${AVRO_HPP} -n IG
    DEPENDS ${AVRO_SCHEMA}
    COMMENT "Generating C++ classes from Avro schema"
)

# Add custom target for Avro generation
add_custom_target(generate_avro DEPENDS ${AVRO_HPP})

# Consumer executable
add_executable(kafka_consumer 
    WebSocketServer.cpp
    KafkaConsumer.cpp
)

# Producer executable
add_executable(kafka_producer
    KafkaProducer.cpp
)

# Add dependency on Avro generation
add_dependencies(kafka_producer generate_avro)
add_dependencies(kafka_consumer generate_avro)

# Common include directories
set(COMMON_INCLUDE_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}
    /usr/include
    /usr/local/include
)

# Set include directories for consumer
target_include_directories(kafka_consumer PRIVATE
    ${COMMON_INCLUDE_DIRS}
)

# Set include directories for producer
target_include_directories(kafka_producer PRIVATE
    ${COMMON_INCLUDE_DIRS}
)

# Link consumer libraries
target_link_libraries(kafka_consumer
    PRIVATE
    ${RDKAFKA_LIBRARY}
    ${AVROCPP_LIBRARY}
    pthread
    boost_system
)

# Link producer libraries
target_link_libraries(kafka_producer
    PRIVATE
    ${RDKAFKA_LIBRARY}
    ${AVROCPP_LIBRARY}
    pthread
)

# Optional: Install targets
install(TARGETS kafka_consumer kafka_producer
    RUNTIME DESTINATION bin
)

# Print status information
message(STATUS "RDKAFKA_LIBRARY: ${RDKAFKA_LIBRARY}")
message(STATUS "AVROCPP_LIBRARY: ${AVROCPP_LIBRARY}")
message(STATUS "AVROGENCPP: ${AVROGENCPP}")