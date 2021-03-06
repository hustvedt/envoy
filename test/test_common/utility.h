#pragma once

#include <stdlib.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "envoy/buffer/buffer.h"
#include "envoy/network/address.h"
#include "envoy/stats/stats.h"

#include "common/http/header_map_impl.h"

#include "test/test_common/printers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
#define EXPECT_THROW_WITH_MESSAGE(statement, expected_exception, message)                          \
  try {                                                                                            \
    statement;                                                                                     \
    ADD_FAILURE() << "Exception should take place. It did not.";                                   \
  } catch (expected_exception & e) {                                                               \
    EXPECT_EQ(message, std::string(e.what()));                                                     \
  }

// Random number generator which logs its seed to stderr.  To repeat a test run with a non-zero seed
// one can run the test with --test_arg=--gtest_filter=[seed]
class TestRandomGenerator {
public:
  TestRandomGenerator();

  uint64_t random();

private:
  const int32_t seed_;
  std::ranlux48 generator_;
};

class TestUtility {
public:
  /**
   * Compare 2 buffers.
   * @param lhs supplies buffer 1.
   * @param rhs supplies buffer 2.
   * @return TRUE if the buffers are equal, false if not.
   */
  static bool buffersEqual(const Buffer::Instance& lhs, const Buffer::Instance& rhs);

  /**
   * Convert a buffer to a string.
   * @param buffer supplies the buffer to convert.
   * @return std::string the converted string.
   */
  static std::string bufferToString(const Buffer::Instance& buffer);

  /**
   * Find a counter in a stats store.
   * @param store supplies the stats store.
   * @param name supplies the name to search for.
   * @return Stats::CounterSharedPtr the counter or nullptr if there is none.
   */
  static Stats::CounterSharedPtr findCounter(Stats::Store& store, const std::string& name);

  /**
   * Find a gauge in a stats store.
   * @param store supplies the stats store.
   * @param name supplies the name to search for.
   * @return Stats::GaugeSharedPtr the gauge or nullptr if there is none.
   */
  static Stats::GaugeSharedPtr findGauge(Stats::Store& store, const std::string& name);

  /**
   * Convert a string list of IP addresses into a list of network addresses usable for DNS
   * response testing.
   */
  static std::list<Network::Address::InstanceConstSharedPtr>
  makeDnsResponse(const std::list<std::string>& addresses);

  /**
   * List files in a given directory path
   *
   * @param path directory path to list
   * @param recursive whether or not to traverse subdirectories
   * @return std::vector<std::string> filenames
   */
  static std::vector<std::string> listFiles(const std::string& path, bool recursive);

  /**
   * Compare two protos of the same type for equality.
   *
   * @param lhs proto on LHS.
   * @param rhs proto on RHS.
   * @return bool indicating whether the protos are equal. Type name and string serialization are
   *         used for equality testing.
   */
  template <class ProtoType> static bool protoEqual(ProtoType lhs, ProtoType rhs) {
    return lhs.GetTypeName() == rhs.GetTypeName() &&
           lhs.SerializeAsString() == rhs.SerializeAsString();
  }
};

/**
 * This utility class wraps the common case of having a cross-thread "one shot" ready condition.
 */
class ConditionalInitializer {
public:
  /**
   * Set the conditional to ready.
   */
  void setReady();

  /**
   * Block until the conditional is ready, will return immediately if it is already ready. This
   * routine will also reset ready_ so that the initializer can be used again. setReady() should
   * only be called once in between a call to waitReady().
   */
  void waitReady();

private:
  std::condition_variable cv_;
  std::mutex mutex_;
  bool ready_{false};
};

class ScopedFdCloser {
public:
  ScopedFdCloser(int fd);
  ~ScopedFdCloser();

private:
  int fd_;
};

namespace Http {

/**
 * A test version of HeaderMapImpl that adds some niceties around letting us use
 * std::string instead of always doing LowerCaseString() by hand.
 */
class TestHeaderMapImpl : public HeaderMapImpl {
public:
  TestHeaderMapImpl();
  TestHeaderMapImpl(const std::initializer_list<std::pair<std::string, std::string>>& values);
  TestHeaderMapImpl(const HeaderMap& rhs);

  using HeaderMapImpl::addCopy;
  void addCopy(const std::string& key, const std::string& value);
  std::string get_(const std::string& key);
  std::string get_(const LowerCaseString& key);
  bool has(const std::string& key);
  bool has(const LowerCaseString& key);
};

} // namespace Http

MATCHER_P(ProtoEq, rhs, "") { return TestUtility::protoEqual(arg, rhs); }

MATCHER_P(RepeatedProtoEq, rhs, "") {
  if (arg.size() != rhs.size()) {
    return false;
  }

  for (int i = 0; i < arg.size(); ++i) {
    if (!TestUtility::protoEqual(arg[i], rhs[i])) {
      return false;
    }
  }

  return true;
}

} // Envoy
