#include <unity.h>
#include "core/AnalyzerMath.h"

void test_percent_clamping() {
    TEST_ASSERT_EQUAL_UINT8(0, AnalyzerMath::clampPercent(-1));
    TEST_ASSERT_EQUAL_UINT8(42, AnalyzerMath::clampPercent(42));
    TEST_ASSERT_EQUAL_UINT8(100, AnalyzerMath::clampPercent(130));
}

void test_exponential_average() {
    TEST_ASSERT_EQUAL_UINT8(25, AnalyzerMath::ema(0, 100));
    TEST_ASSERT_EQUAL_UINT8(62, AnalyzerMath::ema(50, 100));
}

void test_baseline_delta_never_underflows() {
    TEST_ASSERT_EQUAL_UINT8(0, AnalyzerMath::deltaAboveBaseline(30, 50));
    TEST_ASSERT_EQUAL_UINT8(35, AnalyzerMath::deltaAboveBaseline(75, 40));
}

void test_confidence_rewards_samples_receivers_and_baseline() {
    const uint8_t shallow = AnalyzerMath::observationConfidence(12, 1, false);
    const uint8_t deep = AnalyzerMath::observationConfidence(60, 2, true);
    TEST_ASSERT_GREATER_THAN(shallow, deep);
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(100, deep);
}

void test_event_run_uses_hysteresis() {
    uint8_t run = AnalyzerMath::nextEventRun(0, 65, 60, 10);
    TEST_ASSERT_EQUAL_UINT8(1, run);
    run = AnalyzerMath::nextEventRun(run, 55, 60, 10);
    TEST_ASSERT_EQUAL_UINT8(1, run);
    run = AnalyzerMath::nextEventRun(run, 50, 60, 10);
    TEST_ASSERT_EQUAL_UINT8(0, run);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_percent_clamping);
    RUN_TEST(test_exponential_average);
    RUN_TEST(test_baseline_delta_never_underflows);
    RUN_TEST(test_confidence_rewards_samples_receivers_and_baseline);
    RUN_TEST(test_event_run_uses_hysteresis);
    return UNITY_END();
}
