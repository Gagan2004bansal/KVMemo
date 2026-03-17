#pragma once
/**
 * @file test_protocol.cpp
 * @brief Comprehensive unit tests for KVMemo protocol components.
 *
 * This test suite validates all major and minor test cases for:
 * - Request parsing, construction, and edge cases
 * - Response creation and status handling
 * - Buffer operations (append, consume, clear)
 * - Protocol framing (frame extraction, delimiter handling)
 * - Parser functionality (tokenization, edge cases)
 * - Serializer output formatting and correctness
 * - Status/Error handling throughout the protocol stack
 * - Concurrency safety assumptions
 *
 * Copyright © 2026 Gagan Bansal
 * ALL RIGHTS RESERVED
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>

#include "../src/protocol/request.h"
#include "../src/protocol/response.h"
#include "../src/protocol/buffer.h"
#include "../src/protocol/framing.h"
#include "../src/protocol/parser.h"
#include "../src/protocol/serializer.h"
#include "../src/common/status.h"

using namespace kvmemo;
using namespace kvmemo::protocol;
using namespace kvmemo::common;

/**
 * ============================================================
 * Test Framework & Utilities
 * ============================================================
 */

class TestRunner {
public:
    TestRunner() : passed_(0), failed_(0) {}

    void Run(const std::string& test_name, std::function<void()> test_func) {
        try {
            test_func();
            passed_++;
            std::cout << "[PASS] " << test_name << std::endl;
        } catch (const std::exception& e) {
            failed_++;
            std::cout << "[FAIL] " << test_name << " - " << e.what() << std::endl;
        }
    }

    void Report() const {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "FINAL TEST REPORT" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "✓ Passed: " << passed_ << std::endl;
        std::cout << "✗ Failed: " << failed_ << std::endl;
        std::cout << "Total:  " << (passed_ + failed_) << std::endl;
        std::cout << std::string(60, '=') << std::endl;
    }

    bool AllPassed() const { return failed_ == 0; }

private:
    int passed_;
    int failed_;
};

void AssertTrue(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void AssertFalse(bool condition, const std::string& message) {
    if (condition) {
        throw std::runtime_error(message);
    }
}

void AssertEqual(const std::string& expected, const std::string& actual, const std::string& message) {
    if (expected != actual) {
        throw std::runtime_error(message + " - Expected: '" + expected + "', Got: '" + actual + "'");
    }
}

void AssertEqual(size_t expected, size_t actual, const std::string& message) {
    if (expected != actual) {
        throw std::runtime_error(message + " - Expected: " + std::to_string(expected) + ", Got: " + std::to_string(actual));
    }
}

void AssertEqual(int expected, int actual, const std::string& message) {
    if (expected != actual) {
        throw std::runtime_error(message + " - Expected: " + std::to_string(expected) + ", Got: " + std::to_string(actual));
    }
}

void AssertThrows(std::function<void()> func, const std::string& message) {
    try {
        func();
        throw std::runtime_error(message + " - Expected exception but none was thrown");
    } catch (const std::exception&) {
        // Expected
    }
}

/**
 * ============================================================
 * REQUEST TESTS - MAJOR TEST CASES
 * ============================================================
 */

void TestRequestConstructionBasic() {
    Request req("SET", {"key1", "value1"});
    AssertEqual("SET", req.Command(), "Command should be 'SET'");
    AssertEqual(size_t(2), req.ArgCount(), "Should have 2 arguments");
    AssertEqual("key1", req.Arg(0), "First argument should be 'key1'");
    AssertEqual("value1", req.Arg(1), "Second argument should be 'value1'");
}

void TestRequestConstructionEmpty() {
    Request req;
    AssertTrue(req.Empty(), "Default request should be empty");
    AssertEqual("", req.Command(), "Empty command string");
    AssertEqual(size_t(0), req.ArgCount(), "Empty request has zero arguments");
}

void TestRequestConstructionSingleArgument() {
    Request req("GET", {"mykey"});
    AssertEqual("GET", req.Command(), "Command mismatch");
    AssertEqual(size_t(1), req.ArgCount(), "Should have 1 argument");
    AssertEqual("mykey", req.Arg(0), "Argument mismatch");
}

void TestRequestConstructionMultipleArguments() {
    Request req("MSET", {"k1", "v1", "k2", "v2", "k3", "v3"});
    AssertEqual(size_t(6), req.ArgCount(), "Should have 6 arguments");
    AssertEqual("k1", req.Arg(0), "First key mismatch");
    AssertEqual("v3", req.Arg(5), "Last value mismatch");
}

void TestRequestArgumentAccess() {
    Request req("MGET", {"key1", "key2", "key3"});
    const auto& args = req.Args();
    AssertEqual(size_t(3), args.size(), "Args vector size mismatch");
    AssertEqual("key1", args[0], "Args[0] mismatch");
    AssertEqual("key2", args[1], "Args[1] mismatch");
    AssertEqual("key3", args[2], "Args[2] mismatch");
}

void TestRequestArgumentOutOfRangeAccess() {
    Request req("GET", {"key"});
    AssertThrows([&req]() { req.Arg(10); }, "Should throw out_of_range exception");
}

void TestRequestArgumentZeroIndexBoundary() {
    Request req("TEST", {"arg0"});
    AssertEqual("arg0", req.Arg(0), "Should access valid index 0");
}

void TestRequestArgumentLastIndexBoundary() {
    Request req("TEST", {"arg0", "arg1", "arg2"});
    AssertEqual("arg2", req.Arg(2), "Should access valid last index");
    AssertThrows([&req]() { req.Arg(3); }, "Should throw when accessing beyond last index");
}

void TestRequestCopySemantics() {
    Request req1("SET", {"key", "value"});
    Request req2 = req1;
    
    AssertEqual("SET", req2.Command(), "Copy: Command mismatch");
    AssertEqual(size_t(2), req2.ArgCount(), "Copy: Argument count mismatch");
    AssertEqual("key", req2.Arg(0), "Copy: First argument mismatch");
}

void TestRequestMoveSemantics() {
    Request req1("DEL", {"key1", "key2"});
    Request req2 = std::move(req1);
    
    AssertEqual("DEL", req2.Command(), "Move: Command mismatch");
    AssertEqual(size_t(2), req2.ArgCount(), "Move: Argument count mismatch");
}

void TestRequestNonEmptyCheck() {
    Request req1;
    AssertTrue(req1.Empty(), "Default request should be empty");
    
    Request req2("CMD", {});
    AssertFalse(req2.Empty(), "Request with command should not be empty");
}

void TestRequestWithSpecialCharactersInArguments() {
    Request req("SET", {"key-with-dash", "value@special#chars"});
    AssertEqual("key-with-dash", req.Arg(0), "Should handle special characters in arguments");
    AssertEqual("value@special#chars", req.Arg(1), "Should preserve special characters");
}

void TestRequestLargeNumberOfArguments() {
    std::vector<std::string> args;
    for (int i = 0; i < 100; ++i) {
        args.push_back("arg" + std::to_string(i));
    }
    Request req("BULK_CMD", args);
    AssertEqual(size_t(100), req.ArgCount(), "Should handle 100 arguments");
    AssertEqual("arg50", req.Arg(50), "Should access middle argument");
    AssertEqual("arg99", req.Arg(99), "Should access last argument");
}

/**
 * ============================================================
 * RESPONSE TESTS - MAJOR & MINOR TEST CASES
 * ============================================================
 */

void TestResponseConstructionDefault() {
    Response resp;
    AssertTrue(resp.IsOk(), "Default response should be OK");
    AssertEqual("", resp.Message(), "Default message should be empty");
}

void TestResponseConstructionOkWithMessage() {
    Response resp = Response::Ok("Success message");
    AssertTrue(resp.IsOk(), "Should be OK status");
    AssertFalse(resp.IsError(), "Should not be error status");
    AssertEqual("Success message", resp.Message(), "Message mismatch");
}

void TestResponseConstructionErrorWithMessage() {
    Response resp = Response::Error("Error occurred");
    AssertFalse(resp.IsOk(), "Should not be OK");
    AssertTrue(resp.IsError(), "Should be error status");
    AssertEqual("Error occurred", resp.Message(), "Error message mismatch");
}

void TestResponseOkEmptyMessage() {
    Response resp = Response::Ok("");
    AssertTrue(resp.IsOk(), "Should be OK");
    AssertEqual("", resp.Message(), "Message should be empty");
}

void TestResponseStatusGetter() {
    Response ok_resp = Response::Ok("test");
    Response err_resp = Response::Error("error");
    
    AssertEqual(int(ResponseStatus::Ok), int(ok_resp.Status()), "Status should be Ok");
    AssertEqual(int(ResponseStatus::Error), int(err_resp.Status()), "Status should be Error");
}

void TestResponseCopySemantics() {
    Response resp1 = Response::Ok("Original");
    Response resp2 = resp1;
    
    AssertTrue(resp2.IsOk(), "Copy: Should be OK");
    AssertEqual("Original", resp2.Message(), "Copy: Message mismatch");
}

void TestResponseMoveSemantics() {
    Response resp1 = Response::Error("Error message");
    Response resp2 = std::move(resp1);
    
    AssertTrue(resp2.IsError(), "Move: Should be error");
    AssertEqual("Error message", resp2.Message(), "Move: Message mismatch");
}

void TestResponseWithSpecialCharactersInMessage() {
    Response resp = Response::Error("Error: Key not found!@#$%");
    AssertEqual("Error: Key not found!@#$%", resp.Message(), "Should preserve special characters");
}

void TestResponseWithVeryLongMessage() {
    std::string long_msg(1000, 'a');
    Response resp = Response::Ok(long_msg);
    AssertEqual(long_msg, resp.Message(), "Should handle very long message");
}

/**
 * ============================================================
 * BUFFER TESTS - MAJOR & MINOR TEST CASES
 * ============================================================
 */

void TestBufferAppendCharArray() {
    Buffer buf;
    const char* data = "Hello";
    buf.Append(data, 5);
    
    AssertEqual(size_t(5), buf.ReadableBytes(), "Should have 5 readable bytes");
    AssertEqual("Hello", std::string(buf.Data(), buf.ReadableBytes()), "Data mismatch");
}

void TestBufferAppendString() {
    Buffer buf;
    std::string data = "World";
    buf.Append(data);
    
    AssertEqual(size_t(5), buf.ReadableBytes(), "Should have 5 readable bytes");
}

void TestBufferAppendMultiple() {
    Buffer buf;
    buf.Append("Hello", 5);
    buf.Append(" ");
    buf.Append("World");
    
    AssertEqual(size_t(11), buf.ReadableBytes(), "Should have 11 readable bytes");
}

void TestBufferReadableBytes() {
    Buffer buf;
    AssertEqual(size_t(0), buf.ReadableBytes(), "Empty buffer has zero readable bytes");
    
    buf.Append("test", 4);
    AssertEqual(size_t(4), buf.ReadableBytes(), "Should have 4 readable bytes");
}

void TestBufferDataPointer() {
    Buffer buf;
    buf.Append("data");
    
    const char* ptr = buf.Data();
    AssertTrue(ptr != nullptr, "Data pointer should not be null");
    AssertEqual("data", std::string(ptr, buf.ReadableBytes()), "Data content mismatch");
}

void TestBufferConsume() {
    Buffer buf;
    buf.Append("Hello", 5);
    
    buf.Consume(3);
    AssertEqual(size_t(2), buf.ReadableBytes(), "Should have 2 bytes after consuming 3");
    AssertEqual("lo", std::string(buf.Data(), buf.ReadableBytes()), "Remaining data mismatch");
}

void TestBufferConsumeAll() {
    Buffer buf;
    buf.Append("test", 4);
    buf.Consume(4);
    
    AssertEqual(size_t(0), buf.ReadableBytes(), "Buffer should be empty");
}

void TestBufferConsumeBeyondReadable() {
    Buffer buf;
    buf.Append("hi", 2);
    
    AssertThrows([&buf]() { buf.Consume(10); }, "Should throw when consuming beyond readable data");
}

void TestBufferClear() {
    Buffer buf;
    buf.Append("some data", 9);
    buf.Clear();
    
    AssertEqual(size_t(0), buf.ReadableBytes(), "Buffer should be empty after clear");
}

void TestBufferClearEmptyBuffer() {
    Buffer buf;
    buf.Clear();
    AssertEqual(size_t(0), buf.ReadableBytes(), "Clear on empty buffer should still be valid");
}

void TestBufferLargeBinaryData() {
    Buffer buf;
    std::vector<char> large_data(10000);
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<char>(i % 256);
    }
    buf.Append(large_data.data(), large_data.size());
    
    AssertEqual(size_t(10000), buf.ReadableBytes(), "Should store large binary data");
}

void TestBufferReusageAfterClear() {
    Buffer buf;
    buf.Append("first", 5);
    buf.Clear();
    buf.Append("second", 6);
    
    AssertEqual(size_t(6), buf.ReadableBytes(), "Should reuse buffer after clear");
    AssertEqual("second", std::string(buf.Data(), buf.ReadableBytes()), "Should have new data");
}

void TestBufferAppendAfterConsume() {
    Buffer buf;
    buf.Append("Hello", 5);
    buf.Consume(2);
    buf.Append(" World", 6);
    
    AssertEqual(size_t(9), buf.ReadableBytes(), "Should have 9 bytes (3 + 6)");
}

/**
 * ============================================================
 * FRAMING TESTS - MAJOR & MINOR TEST CASES
 * ============================================================
 */

void TestFramingSimpleFrame() {
    Buffer buf;
    buf.Append("SET key value\r\n");
    
    std::string frame;
    bool found = Framing::NextFrame(buf, frame);
    
    AssertTrue(found, "Should find frame");
    AssertEqual("SET key value", frame, "Frame content mismatch");
    AssertEqual(size_t(0), buf.ReadableBytes(), "Buffer should be empty after extracting frame");
}

void TestFramingNoCompleteFrame() {
    Buffer buf;
    buf.Append("incomplete frame", 15);
    
    std::string frame;
    bool found = Framing::NextFrame(buf, frame);
    
    AssertFalse(found, "Should not find frame without delimiter");
    AssertEqual(size_t(15), buf.ReadableBytes(), "Data should remain in buffer");
}

void TestFramingMultipleFrames() {
    Buffer buf;
    buf.Append("FRAME1\r\nFRAME2\r\n");
    
    std::string frame1, frame2;
    bool found1 = Framing::NextFrame(buf, frame1);
    bool found2 = Framing::NextFrame(buf, frame2);
    
    AssertTrue(found1, "Should find first frame");
    AssertTrue(found2, "Should find second frame");
    AssertEqual("FRAME1", frame1, "First frame mismatch");
    AssertEqual("FRAME2", frame2, "Second frame mismatch");
}

void TestFramingEmptyFrame() {
    Buffer buf;
    buf.Append("\r\n");
    
    std::string frame;
    bool found = Framing::NextFrame(buf, frame);
    
    AssertTrue(found, "Should find empty frame");
    AssertEqual("", frame, "Frame should be empty string");
}

void TestFramingFrameWithSpecialCharacters() {
    Buffer buf;
    buf.Append("CMD @#$%^&*()\r\n");
    
    std::string frame;
    bool found = Framing::NextFrame(buf, frame);
    
    AssertTrue(found, "Should extract frame with special characters");
    AssertEqual("CMD @#$%^&*()", frame, "Special characters should be preserved");
}

void TestFramingLargeFrame() {
    Buffer buf;
    std::string large_cmd(10000, 'A');
    large_cmd += "\r\n";
    buf.Append(large_cmd);
    
    std::string frame;
    bool found = Framing::NextFrame(buf, frame);
    
    AssertTrue(found, "Should find large frame");
    AssertEqual(size_t(10000), frame.size(), "Large frame size mismatch");
}

void TestFramingIncompleteDelimiter() {
    Buffer buf;
    buf.Append("command\r", 8);
    
    std::string frame;
    bool found = Framing::NextFrame(buf, frame);
    
    AssertFalse(found, "Should not find frame with incomplete delimiter");
}

void TestFramingBufferConsumption() {
    Buffer buf;
    buf.Append("Frame1\r\nExtra");
    
    std::string frame;
    Framing::NextFrame(buf, frame);
    
    AssertEqual("Extra", std::string(buf.Data(), buf.ReadableBytes()), "Extra data should remain");
}

/**
 * ============================================================
 * PARSER TESTS - MAJOR & MINOR TEST CASES
 * ============================================================
 */

void TestParserSingleCommand() {
    Request req = Parser::Parse("GET");
    AssertEqual("GET", req.Command(), "Command mismatch");
    AssertEqual(size_t(0), req.ArgCount(), "Should have zero arguments");
}

void TestParserCommandWithOneArgument() {
    Request req = Parser::Parse("GET mykey");
    AssertEqual("GET", req.Command(), "Command should be GET");
    AssertEqual(size_t(1), req.ArgCount(), "Should have 1 argument");
    AssertEqual("mykey", req.Arg(0), "Argument mismatch");
}

void TestParserCommandWithMultipleArguments() {
    Request req = Parser::Parse("MSET key1 val1 key2 val2");
    AssertEqual("MSET", req.Command(), "Command should be MSET");
    AssertEqual(size_t(4), req.ArgCount(), "Should have 4 arguments");
    AssertEqual("key1", req.Arg(0), "First key mismatch");
    AssertEqual("val2", req.Arg(3), "Last value mismatch");
}

void TestParserEmptyInput() {
    AssertThrows([]() { Parser::Parse(""); }, "Should throw on empty input");
    AssertThrows([]() { Parser::Parse("   "); }, "Should throw on whitespace-only input");
}

void TestParserLeadingTrailingWhitespace() {
    Request req = Parser::Parse("  GET  mykey  ");
    AssertEqual("GET", req.Command(), "Should skip leading/trailing whitespace");
    AssertEqual(size_t(1), req.ArgCount(), "Should have 1 argument");
}

void TestParserMultipleSpaces() {
    Request req = Parser::Parse("SET    key    value");
    AssertEqual("SET", req.Command(), "Should handle multiple spaces");
    AssertEqual(size_t(2), req.ArgCount(), "Should have 2 arguments");
}

void TestParserCommandCaseSensitivity() {
    Request req = Parser::Parse("get mykey");
    AssertEqual("get", req.Command(), "Should preserve command case");
}

void TestParserArgumentsPreservation() {
    Request req = Parser::Parse("CMD arg1 arg2 arg3");
    const auto& args = req.Args();
    AssertEqual("arg1", args[0], "First argument mismatch");
    AssertEqual("arg2", args[1], "Second argument mismatch");
    AssertEqual("arg3", args[2], "Third argument mismatch");
}

void TestParserSpecialCharactersInArguments() {
    Request req = Parser::Parse("SET key-1 val@123");
    AssertEqual("SET", req.Command(), "Command mismatch");
    AssertEqual("key-1", req.Arg(0), "Should preserve hyphen");
    AssertEqual("val@123", req.Arg(1), "Should preserve special characters");
}

void TestParserLargeCommandLine() {
    std::string large_cmd = "BULK_CMD";
    for (int i = 0; i < 100; ++i) {
        large_cmd += " arg" + std::to_string(i);
    }
    
    Request req = Parser::Parse(large_cmd);
    AssertEqual("BULK_CMD", req.Command(), "Command mismatch");
    AssertEqual(size_t(100), req.ArgCount(), "Should parse 100 arguments");
}

void TestParserTabSeparators() {
    Request req = Parser::Parse("SET\tkey\tvalue");
    AssertEqual("SET", req.Command(), "Should handle tab separators");
    AssertEqual(size_t(2), req.ArgCount(), "Should parse arguments separated by tabs");
}

/**
 * ============================================================
 * SERIALIZER TESTS - MAJOR & MINOR TEST CASES
 * ============================================================
 */

void TestSerializerOkResponseEmpty() {
    Response resp = Response::Ok("");
    std::string serialized = Serializer::Serialize(resp);
    
    AssertEqual("+OK\r\n", serialized, "OK response serialization mismatch");
}

void TestSerializerOkResponseWithMessage() {
    Response resp = Response::Ok("Success");
    std::string serialized = Serializer::Serialize(resp);
    
    AssertEqual("$7\r\nSuccess\r\n", serialized, "Bulk string serialization mismatch");
}

void TestSerializerErrorResponse() {
    Response resp = Response::Error("Key not found");
    std::string serialized = Serializer::Serialize(resp);
    
    // Format: -ERR<message>\r\n
    AssertTrue(serialized.find("-ERR") != std::string::npos, "Should contain error prefix");
    AssertTrue(serialized.find("Key not found") != std::string::npos, "Should contain error message");
    AssertTrue(serialized.find("\r\n") != std::string::npos, "Should contain CRLF ending");
}

void TestSerializerBulkStringLength() {
    Response resp = Response::Ok("Hello");
    std::string serialized = Serializer::Serialize(resp);
    
    AssertEqual("$5\r\nHello\r\n", serialized, "Length encoding mismatch");
}

void TestSerializerBulkStringEmpty() {
    // Empty message is treated as simple OK
    Response resp = Response::Ok("");
    std::string serialized = Serializer::Serialize(resp);
    
    AssertEqual("+OK\r\n", serialized, "Empty message should use simple OK");
}

void TestSerializerErrorWithEmptyMessage() {
    Response resp = Response::Error("");
    std::string serialized = Serializer::Serialize(resp);
    
    AssertTrue(serialized.find("-ERR") != std::string::npos, "Should have error prefix");
}

void TestSerializerSpecialCharactersInMessage() {
    Response resp = Response::Ok("Message with @#$%^&*()");
    std::string serialized = Serializer::Serialize(resp);
    
    AssertTrue(serialized.find("@#$%^&*()") != std::string::npos, "Should preserve special characters");
}

void TestSerializerLongMessage() {
    std::string long_msg(1000, 'X');
    Response resp = Response::Ok(long_msg);
    std::string serialized = Serializer::Serialize(resp);
    
    AssertTrue(serialized.find("$1000") != std::string::npos, "Should encode correct length");
    AssertTrue(serialized.find(long_msg) != std::string::npos, "Should include full message");
}

void TestSerializerCRLFEncoding() {
    Response resp = Response::Ok("test");
    std::string serialized = Serializer::Serialize(resp);
    
    // Check for proper CRLF endings
    AssertTrue(serialized.find("\r\n") != std::string::npos, "Should have CRLF");
}

void TestSerializerResponseStatus() {
    Response ok_resp = Response::Ok("OK");
    Response err_resp = Response::Error("Error");
    
    std::string ok_ser = Serializer::Serialize(ok_resp);
    std::string err_ser = Serializer::Serialize(err_resp);
    
    AssertTrue(ok_ser[0] == '$' || ok_ser[0] == '+', "OK response should start with $ or +");
    AssertTrue(err_ser[0] == '-', "Error response should start with -");
}

/**
 * ============================================================
 * STATUS / ERROR HANDLING TESTS
 * ============================================================
 */

void TestStatusOk() {
    Status status = Status::Ok();
    AssertTrue(status.ok(), "Status should be OK");
    AssertEqual("OK", status.ToString(), "OK status string mismatch");
}

void TestStatusProtocolError() {
    Status status = Status::ProtocolError("Invalid frame format");
    AssertFalse(status.ok(), "Status should not be OK");
    AssertTrue(status.ToString().find("PROTOCOL_ERROR") != std::string::npos, "Should contain error code");
}

void TestStatusNetworkError() {
    Status status = Status::NetworkError("Connection lost");
    AssertFalse(status.ok(), "Status should not be OK");
}

void TestStatusEquality() {
    Status s1 = Status::ProtocolError("Error");
    Status s2 = Status::ProtocolError("Error");
    Status s3 = Status::NetworkError("Different");
    
    AssertTrue(s1 == s2, "Equal statuses should match");
    AssertTrue(s1 != s3, "Different statuses should not match");
}

void TestStatusInequality() {
    Status s1 = Status::Ok();
    Status s2 = Status::InternalError("Error");
    
    AssertTrue(s1 != s2, "Different statuses should not be equal");
}

/**
 * ============================================================
 * INTEGRATION TESTS - MAJOR TEST CASES
 * ============================================================
 */

void TestIntegrationParseAndSerializeRequest() {
    // Parse request
    Request req = Parser::Parse("SET mykey myvalue");
    
    AssertEqual("SET", req.Command(), "Parsed command mismatch");
    AssertEqual(size_t(2), req.ArgCount(), "Parsed argument count mismatch");
}

void TestIntegrationFramingAndParsing() {
    // Create buffer with framed data
    Buffer buf;
    buf.Append("GET mykey\r\n");
    
    // Extract frame
    std::string frame;
    bool found = Framing::NextFrame(buf, frame);
    
    // Parse frame
    Request req = Parser::Parse(frame);
    
    AssertTrue(found, "Should find frame");
    AssertEqual("GET", req.Command(), "Parsed command mismatch");
    AssertEqual("mykey", req.Arg(0), "Parsed argument mismatch");
}

void TestIntegrationBufferFramingParsing() {
    Buffer buf;
    buf.Append("SET key1 value1\r\nGET key2\r\n");
    
    // First frame
    std::string frame1;
    AssertTrue(Framing::NextFrame(buf, frame1), "Should extract first frame");
    Request req1 = Parser::Parse(frame1);
    AssertEqual("SET", req1.Command(), "First command mismatch");
    
    // Second frame
    std::string frame2;
    AssertTrue(Framing::NextFrame(buf, frame2), "Should extract second frame");
    Request req2 = Parser::Parse(frame2);
    AssertEqual("GET", req2.Command(), "Second command mismatch");
}

void TestIntegrationResponseSerialization() {
    Response resp = Response::Ok("Success");
    std::string wire_format = Serializer::Serialize(resp);
    
    AssertTrue(wire_format.find("$") != std::string::npos, "Should be bulk string");
    AssertTrue(wire_format.find("Success") != std::string::npos, "Should contain message");
    AssertTrue(wire_format.find("\r\n") != std::string::npos, "Should have CRLF");
}

/**
 * ============================================================
 * EDGE CASE TESTS
 * ============================================================
 */

void TestEdgeCaseEmptyCommandLine() {
    AssertThrows([]() { Parser::Parse(""); }, "Empty command should throw");
}

void TestEdgeCaseWhitespaceOnly() {
    AssertThrows([]() { Parser::Parse("   \t  "); }, "Whitespace-only should throw");
}

void TestEdgeCaseVeryLongCommand() {
    std::string long_cmd(10000, 'A');
    Request req = Parser::Parse(long_cmd);
    AssertEqual(long_cmd, req.Command(), "Should handle very long command");
}

void TestEdgeCaseBufferExactBoundary() {
    Buffer buf;
    buf.Append("test");
    buf.Consume(4);
    AssertEqual(size_t(0), buf.ReadableBytes(), "Exact boundary consumption");
}

void TestEdgeCaseMultipleConsecutiveFrames() {
    Buffer buf;
    for (int i = 0; i < 10; ++i) {
        buf.Append("FRAME\r\n");
    }
    
    for (int i = 0; i < 10; ++i) {
        std::string frame;
        AssertTrue(Framing::NextFrame(buf, frame), "Should find frame " + std::to_string(i));
        AssertEqual("FRAME", frame, "Frame content mismatch " + std::to_string(i));
    }
}

/**
 * ============================================================
 * Main Test Entry Point
 * ============================================================
 */

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "KVMemo Protocol Test Suite - Comprehensive Testing" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << std::endl;

    TestRunner runner;

    // REQUEST TESTS
    std::cout << "\n>>> REQUEST TESTS <<<" << std::endl;
    runner.Run("TestRequestConstructionBasic", TestRequestConstructionBasic);
    runner.Run("TestRequestConstructionEmpty", TestRequestConstructionEmpty);
    runner.Run("TestRequestConstructionSingleArgument", TestRequestConstructionSingleArgument);
    runner.Run("TestRequestConstructionMultipleArguments", TestRequestConstructionMultipleArguments);
    runner.Run("TestRequestArgumentAccess", TestRequestArgumentAccess);
    runner.Run("TestRequestArgumentOutOfRangeAccess", TestRequestArgumentOutOfRangeAccess);
    runner.Run("TestRequestArgumentZeroIndexBoundary", TestRequestArgumentZeroIndexBoundary);
    runner.Run("TestRequestArgumentLastIndexBoundary", TestRequestArgumentLastIndexBoundary);
    runner.Run("TestRequestCopySemantics", TestRequestCopySemantics);
    runner.Run("TestRequestMoveSemantics", TestRequestMoveSemantics);
    runner.Run("TestRequestNonEmptyCheck", TestRequestNonEmptyCheck);
    runner.Run("TestRequestWithSpecialCharactersInArguments", TestRequestWithSpecialCharactersInArguments);
    runner.Run("TestRequestLargeNumberOfArguments", TestRequestLargeNumberOfArguments);

    // RESPONSE TESTS
    std::cout << "\n>>> RESPONSE TESTS <<<" << std::endl;
    runner.Run("TestResponseConstructionDefault", TestResponseConstructionDefault);
    runner.Run("TestResponseConstructionOkWithMessage", TestResponseConstructionOkWithMessage);
    runner.Run("TestResponseConstructionErrorWithMessage", TestResponseConstructionErrorWithMessage);
    runner.Run("TestResponseOkEmptyMessage", TestResponseOkEmptyMessage);
    runner.Run("TestResponseStatusGetter", TestResponseStatusGetter);
    runner.Run("TestResponseCopySemantics", TestResponseCopySemantics);
    runner.Run("TestResponseMoveSemantics", TestResponseMoveSemantics);
    runner.Run("TestResponseWithSpecialCharactersInMessage", TestResponseWithSpecialCharactersInMessage);
    runner.Run("TestResponseWithVeryLongMessage", TestResponseWithVeryLongMessage);

    // BUFFER TESTS
    std::cout << "\n>>> BUFFER TESTS <<<" << std::endl;
    runner.Run("TestBufferAppendCharArray", TestBufferAppendCharArray);
    runner.Run("TestBufferAppendString", TestBufferAppendString);
    runner.Run("TestBufferAppendMultiple", TestBufferAppendMultiple);
    runner.Run("TestBufferReadableBytes", TestBufferReadableBytes);
    runner.Run("TestBufferDataPointer", TestBufferDataPointer);
    runner.Run("TestBufferConsume", TestBufferConsume);
    runner.Run("TestBufferConsumeAll", TestBufferConsumeAll);
    runner.Run("TestBufferConsumeBeyondReadable", TestBufferConsumeBeyondReadable);
    runner.Run("TestBufferClear", TestBufferClear);
    runner.Run("TestBufferClearEmptyBuffer", TestBufferClearEmptyBuffer);
    runner.Run("TestBufferLargeBinaryData", TestBufferLargeBinaryData);
    runner.Run("TestBufferReusageAfterClear", TestBufferReusageAfterClear);
    runner.Run("TestBufferAppendAfterConsume", TestBufferAppendAfterConsume);

    // FRAMING TESTS
    std::cout << "\n>>> FRAMING TESTS <<<" << std::endl;
    runner.Run("TestFramingSimpleFrame", TestFramingSimpleFrame);
    runner.Run("TestFramingNoCompleteFrame", TestFramingNoCompleteFrame);
    runner.Run("TestFramingMultipleFrames", TestFramingMultipleFrames);
    runner.Run("TestFramingEmptyFrame", TestFramingEmptyFrame);
    runner.Run("TestFramingFrameWithSpecialCharacters", TestFramingFrameWithSpecialCharacters);
    runner.Run("TestFramingLargeFrame", TestFramingLargeFrame);
    runner.Run("TestFramingIncompleteDelimiter", TestFramingIncompleteDelimiter);
    runner.Run("TestFramingBufferConsumption", TestFramingBufferConsumption);

    // PARSER TESTS
    std::cout << "\n>>> PARSER TESTS <<<" << std::endl;
    runner.Run("TestParserSingleCommand", TestParserSingleCommand);
    runner.Run("TestParserCommandWithOneArgument", TestParserCommandWithOneArgument);
    runner.Run("TestParserCommandWithMultipleArguments", TestParserCommandWithMultipleArguments);
    runner.Run("TestParserEmptyInput", TestParserEmptyInput);
    runner.Run("TestParserLeadingTrailingWhitespace", TestParserLeadingTrailingWhitespace);
    runner.Run("TestParserMultipleSpaces", TestParserMultipleSpaces);
    runner.Run("TestParserCommandCaseSensitivity", TestParserCommandCaseSensitivity);
    runner.Run("TestParserArgumentsPreservation", TestParserArgumentsPreservation);
    runner.Run("TestParserSpecialCharactersInArguments", TestParserSpecialCharactersInArguments);
    runner.Run("TestParserLargeCommandLine", TestParserLargeCommandLine);
    runner.Run("TestParserTabSeparators", TestParserTabSeparators);

    // SERIALIZER TESTS
    std::cout << "\n>>> SERIALIZER TESTS <<<" << std::endl;
    runner.Run("TestSerializerOkResponseEmpty", TestSerializerOkResponseEmpty);
    runner.Run("TestSerializerOkResponseWithMessage", TestSerializerOkResponseWithMessage);
    runner.Run("TestSerializerErrorResponse", TestSerializerErrorResponse);
    runner.Run("TestSerializerBulkStringLength", TestSerializerBulkStringLength);
    runner.Run("TestSerializerBulkStringEmpty", TestSerializerBulkStringEmpty);
    runner.Run("TestSerializerErrorWithEmptyMessage", TestSerializerErrorWithEmptyMessage);
    runner.Run("TestSerializerSpecialCharactersInMessage", TestSerializerSpecialCharactersInMessage);
    runner.Run("TestSerializerLongMessage", TestSerializerLongMessage);
    runner.Run("TestSerializerCRLFEncoding", TestSerializerCRLFEncoding);
    runner.Run("TestSerializerResponseStatus", TestSerializerResponseStatus);

    // STATUS / ERROR HANDLING TESTS
    std::cout << "\n>>> STATUS / ERROR HANDLING TESTS <<<" << std::endl;
    runner.Run("TestStatusOk", TestStatusOk);
    runner.Run("TestStatusProtocolError", TestStatusProtocolError);
    runner.Run("TestStatusNetworkError", TestStatusNetworkError);
    runner.Run("TestStatusEquality", TestStatusEquality);
    runner.Run("TestStatusInequality", TestStatusInequality);

    // INTEGRATION TESTS
    std::cout << "\n>>> INTEGRATION TESTS <<<" << std::endl;
    runner.Run("TestIntegrationParseAndSerializeRequest", TestIntegrationParseAndSerializeRequest);
    runner.Run("TestIntegrationFramingAndParsing", TestIntegrationFramingAndParsing);
    runner.Run("TestIntegrationBufferFramingParsing", TestIntegrationBufferFramingParsing);
    runner.Run("TestIntegrationResponseSerialization", TestIntegrationResponseSerialization);

    // EDGE CASE TESTS
    std::cout << "\n>>> EDGE CASE TESTS <<<" << std::endl;
    runner.Run("TestEdgeCaseEmptyCommandLine", TestEdgeCaseEmptyCommandLine);
    runner.Run("TestEdgeCaseWhitespaceOnly", TestEdgeCaseWhitespaceOnly);
    runner.Run("TestEdgeCaseVeryLongCommand", TestEdgeCaseVeryLongCommand);
    runner.Run("TestEdgeCaseBufferExactBoundary", TestEdgeCaseBufferExactBoundary);
    runner.Run("TestEdgeCaseMultipleConsecutiveFrames", TestEdgeCaseMultipleConsecutiveFrames);

    runner.Report();

    return runner.AllPassed() ? 0 : 1;
}

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */