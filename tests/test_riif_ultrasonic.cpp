#include <gtest/gtest.h>
#include "riif_ultrasonic.h"
#include <vector>
#include <cstdint>
#include <string>

class RiifUltrasonicTest : public ::testing::Test {
protected:
    RiifUltrasonic ultrasonic;
};

TEST_F(RiifUltrasonicTest, BasicEncodeDecodeTest) {
    std::string original_message = "Hello RIIF";
    std::vector<int16_t> encoded_signal = ultrasonic.encode(original_message);
    ASSERT_FALSE(encoded_signal.empty());
    std::string decoded_message = ultrasonic.decode(encoded_signal);
    EXPECT_EQ(decoded_message, original_message);
}

TEST_F(RiifUltrasonicTest, EncodeDecodeTransactionData) {
    std::string transaction_id = "TRX12345";
    std::string encryption_key = "SECRET_KEY_789";
    std::string transaction_data = transaction_id + "|" + encryption_key;
    
    std::vector<int16_t> encoded_signal = ultrasonic.encode(transaction_data);
    ASSERT_FALSE(encoded_signal.empty());
    
    std::string decoded_data = ultrasonic.decode(encoded_signal);
    EXPECT_EQ(decoded_data, transaction_data);
}

TEST_F(RiifUltrasonicTest, GenerateEncryptionKey) {
    std::string key = ultrasonic.generateEncryptionKey();
    EXPECT_FALSE(key.empty());
    EXPECT_EQ(key.length(), 32); // Assuming we're using 256-bit keys
}

TEST_F(RiifUltrasonicTest, EncryptDecryptPaymentData) {
    std::string payment_data = "CARD_NUM:1234567890";
    std::string key = ultrasonic.generateEncryptionKey();
    
    std::string encrypted_data = ultrasonic.encrypt(payment_data, key);
    EXPECT_NE(encrypted_data, payment_data);
    
    std::string decrypted_data = ultrasonic.decrypt(encrypted_data, key);
    EXPECT_EQ(decrypted_data, payment_data);
}

TEST_F(RiifUltrasonicTest, UltrasonicFrequencyRange) {
    std::string message = "Test Message";
    std::vector<int16_t> encoded_signal = ultrasonic.encode(message);
    
    // Check if the signal is in the ultrasonic range (above 20kHz)
    // This is a simplified check and might need to be adjusted based on your specific implementation
    EXPECT_TRUE(ultrasonic.isUltrasonicFrequency(encoded_signal));
}

TEST_F(RiifUltrasonicTest, FSKEncodeDecodeTest) {
    std::string original_message = "RIIF FSK";
    std::vector<int16_t> encoded_signal = ultrasonic.encode(original_message);
    ASSERT_FALSE(encoded_signal.empty());
    std::string decoded_message = ultrasonic.decode(encoded_signal);
    EXPECT_EQ(decoded_message, original_message);
    EXPECT_TRUE(ultrasonic.isUltrasonicFrequency(encoded_signal));
}