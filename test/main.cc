#include "gtest/gtest.h"

// pw_unit_test:light requires an event handler to be configured.
#include "pw_unit_test/simple_printing_event_handler.h"

void WriteString(const std::string_view& string, bool newline) {
  printf("%s", string.data());
  if (newline) {
    printf("\n");
  }
}

int main() {
  // The following line has no effect with pw_unit_test_light, but makes this
  // test compatible with upstream GoogleTest.
  testing::InitGoogleTest();

  // Since we are using pw_unit_test:light, set up an event handler.
  pw::unit_test::SimplePrintingEventHandler handler(WriteString);
  pw::unit_test::RegisterEventHandler(&handler);
  return RUN_ALL_TESTS();
}
