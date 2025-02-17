#include <gtest/gtest.h>

#include "lab2/Api.hpp"

namespace lab2 {

std::string GetTempFilePath(const std::string& filename) {
  return "/tmp/" + filename;
}

class CacheTest : public ::testing::Test {
protected:
  std::string tempFilePath;
  int fd;

  void SetUp() override {
    tempFilePath = GetTempFilePath("testfile.tmp");
    unlink(tempFilePath.c_str());
    fd = -1;
  }

  void TearDown() override {
    if (fd != -1) {
      lab2_close(fd);
    }
    unlink(tempFilePath.c_str());
  }
};

// Test opening and closing a file
TEST_F(CacheTest, OpenAndCloseFile) {
  fd = lab2_open(tempFilePath.c_str());
  ASSERT_GE(fd, 0) << "Failed to open file";

  int close_result = lab2_close(fd);
  ASSERT_EQ(close_result, 0) << "Failed to close file";

  fd = -1;  // Mark as closed
}

// Test writing to a file and reading back the data
TEST_F(CacheTest, WriteAndReadData) {
  fd = lab2_open(tempFilePath.c_str());
  ASSERT_GE(fd, 0) << "Failed to open file";

  const char* writeData = "Hello, Block Cache!";
  size_t writeSize = strlen(writeData);
  ssize_t bytesWritten = lab2_write(fd, writeData, writeSize);
  ASSERT_EQ(bytesWritten, static_cast<ssize_t>(writeSize)) << "Write operation failed";

  // Seek back to the beginning of the file
  off_t seekResult = lab2_lseek(fd, 0, SEEK_SET);
  ASSERT_EQ(seekResult, 0) << "Seek operation failed";

  char readBuffer[50] = {0};
  ssize_t bytesRead = lab2_read(fd, readBuffer, writeSize);
  ASSERT_EQ(bytesRead, static_cast<ssize_t>(writeSize)) << "Read operation failed";
  ASSERT_STREQ(readBuffer, writeData) << "Data read does not match data written";

  // Close the file
  int close_result = lab2_close(fd);
  ASSERT_EQ(close_result, 0) << "Failed to close file";

  fd = -1;  // Mark as closed
}

// Test seeking within a file
TEST_F(CacheTest, SeekWithinFile) {
  fd = lab2_open(tempFilePath.c_str());
  ASSERT_GE(fd, 0) << "Failed to open file";

  const char* writeData = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  size_t writeSize = strlen(writeData);
  ssize_t bytesWritten = lab2_write(fd, writeData, writeSize);
  ASSERT_EQ(bytesWritten, static_cast<ssize_t>(writeSize)) << "Write operation failed";

  // Seek to position 10
  off_t seekPos = lab2_lseek(fd, 10, SEEK_SET);
  ASSERT_EQ(seekPos, 10) << "Seek operation failed";

  const char* expectedData = "KLMNOPQRSTUVWXYZ";
  size_t readSize = strlen(expectedData);
  char readBuffer[30] = {0};
  ssize_t bytesRead = lab2_read(fd, readBuffer, readSize);
  ASSERT_EQ(bytesRead, static_cast<ssize_t>(readSize)) << "Read operation failed";
  ASSERT_STREQ(readBuffer, expectedData) << "Data read after seek does not match expected data";

  // Close the file
  int close_result = lab2_close(fd);
  ASSERT_EQ(close_result, 0) << "Failed to close file";

  fd = -1;  // Mark as closed
}

// Test syncing data to disk
TEST_F(CacheTest, FsyncData) {
  fd = lab2_open(tempFilePath.c_str());
  ASSERT_GE(fd, 0) << "Failed to open file";

  const char* writeData = "Data to sync";
  size_t writeSize = strlen(writeData);
  ssize_t bytesWritten = lab2_write(fd, writeData, writeSize);
  ASSERT_EQ(bytesWritten, static_cast<ssize_t>(writeSize)) << "Write operation failed";

  int syncResult = lab2_fsync(fd);
  ASSERT_EQ(syncResult, 0) << "Fsync operation failed";

  // To verify fsync, reopen the file and read the data
  lab2_close(fd);
  fd = lab2_open(tempFilePath.c_str());
  ASSERT_GE(fd, 0) << "Failed to reopen file";

  char readBuffer[50] = {0};
  ssize_t bytesRead = lab2_read(fd, readBuffer, writeSize);
  ASSERT_EQ(bytesRead, static_cast<ssize_t>(writeSize)) << "Read operation failed after fsync";
  ASSERT_STREQ(readBuffer, writeData) << "Data read after fsync does not match data written";

  // Close the file
  int close_result = lab2_close(fd);
  ASSERT_EQ(close_result, 0) << "Failed to close file";

  fd = -1;  // Mark as closed
}

// Test reading from an empty file
TEST_F(CacheTest, ReadFromEmptyFile) {
  fd = lab2_open(tempFilePath.c_str());
  ASSERT_GE(fd, 0) << "Failed to open file";

  char readBuffer[10] = {0};
  ssize_t bytesRead = lab2_read(fd, readBuffer, sizeof(readBuffer));
  ASSERT_EQ(bytesRead, 0) << "Read from empty file should return 0 bytes";

  // Close the file
  int close_result = lab2_close(fd);
  ASSERT_EQ(close_result, 0) << "Failed to close file";

  fd = -1;  // Mark as closed
}

// Test writing and reading large data to trigger cache eviction
TEST_F(CacheTest, CacheEvictionLRU) {
  fd = lab2_open(tempFilePath.c_str());
  ASSERT_GE(fd, 0) << "Failed to open file";

  const size_t blockSize = 4096;
  const size_t numBlocks = 2000;  // Number of blocks to exceed cache capacity

  // Write multiple blocks
  for (size_t i = 0; i < numBlocks; ++i) {
    char writeData[blockSize];
    memset(writeData, 'A' + (i % 26), blockSize);
    ssize_t bytesWritten = lab2_write(fd, writeData, blockSize);
    ASSERT_EQ(bytesWritten, static_cast<ssize_t>(blockSize))
        << "Write operation failed for block " << i;
  }

  // Sync data to disk
  int syncResult = lab2_fsync(fd);
  ASSERT_EQ(syncResult, 0) << "Fsync operation failed";

  // Seek to the beginning
  off_t seekResult = lab2_lseek(fd, 0, SEEK_SET);
  ASSERT_EQ(seekResult, 0) << "Seek operation failed";

  // Read back the data and verify
  for (size_t i = 0; i < numBlocks; ++i) {
    char readData[blockSize];
    ssize_t bytesRead = lab2_read(fd, readData, blockSize);
    ASSERT_EQ(bytesRead, static_cast<ssize_t>(blockSize))
        << "Read operation failed for block " << i;

    char expectedChar = 'A' + (i % 26);
    for (size_t j = 0; j < blockSize; ++j) {
      ASSERT_EQ(readData[j], expectedChar) << "Data mismatch at block " << i << ", byte " << j;
    }
  }

  // Close the file
  int close_result = lab2_close(fd);
  ASSERT_EQ(close_result, 0) << "Failed to close file";

  fd = -1;  // Mark as closed
}

// Test handling of invalid file descriptor
TEST_F(CacheTest, InvalidFileDescriptor) {
  int invalidFd = -1;

  const char* data = "Test Data";
  ssize_t writeResult = lab2_write(invalidFd, data, strlen(data));
  ASSERT_EQ(writeResult, -1) << "Write should fail with invalid file descriptor";

  char buffer[10];
  ssize_t readResult = lab2_read(invalidFd, buffer, sizeof(buffer));
  ASSERT_EQ(readResult, -1) << "Read should fail with invalid file descriptor";

  off_t seekResult = lab2_lseek(invalidFd, 0, SEEK_SET);
  ASSERT_EQ(seekResult, -1) << "Lseek should fail with invalid file descriptor";

  int fsyncResult = lab2_fsync(invalidFd);
  ASSERT_EQ(fsyncResult, -1) << "Fsync should fail with invalid file descriptor";

  int closeResult = lab2_close(invalidFd);
  ASSERT_EQ(closeResult, -1) << "Close should fail with invalid file descriptor";
}

}  // namespace lab2
