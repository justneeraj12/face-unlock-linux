#include "lockscreen_auth.h"

#include <exception>
#include <iostream>
#include <stdexcept>

namespace {

void require(
  bool condition,
  const char* message
) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

face_unlock::LockScreenObservation qualified(
  int elapsed_ms,
  bool matched
) {
  face_unlock::LockScreenObservation observation;
  observation.elapsed_ms = elapsed_ms;
  observation.frame_available = true;
  observation.single_face = true;
  observation.quality_approved = true;
  observation.matched = matched;
  return observation;
}

}  // namespace

int main() {
  try {
    using face_unlock::LockScreenAction;
    using face_unlock::LockScreenAuthSession;
    using face_unlock::LockScreenObservation;

    LockScreenAuthSession low_light;
    LockScreenObservation dark;
    dark.elapsed_ms = 50;
    dark.frame_available = true;
    dark.single_face = true;
    dark.quality_approved = true;
    dark.low_light = true;

    auto result = low_light.observe(dark);
    require(
      result.action == LockScreenAction::RequestIllumination,
      "low light did not request illumination"
    );
    result = low_light.observe(dark);
    require(
      result.reason == "awaiting_illumination",
      "illumination request repeated"
    );
    result = low_light.observe(qualified(60, false));
    require(
      result.reason == "awaiting_illumination" &&
      result.qualified_candidates == 0,
      "candidate counted before illumination acknowledgement"
    );
    result = low_light.acknowledge_illumination(75);
    require(
      result.reason == "illumination_acknowledged",
      "illumination acknowledgement failed"
    );
    result = low_light.acknowledge_illumination(100);
    require(
      result.reason == "illumination_already_acknowledged",
      "duplicate illumination acknowledgement was not idempotent"
    );
    result = low_light.observe(qualified(200, false));
    require(
      result.reason == "illumination_settling" &&
      result.qualified_candidates == 0,
      "settling frame counted as a candidate"
    );

    LockScreenObservation rejected = qualified(300, false);
    rejected.quality_approved = false;
    result = low_light.observe(rejected);
    require(
      result.reason == "quality_rejected" &&
      result.qualified_candidates == 0,
      "rejected frame counted as a candidate"
    );

    result = low_light.observe(qualified(310, false));
    require(
      result.candidates_remaining == 2,
      "first mismatch count is incorrect"
    );
    result = low_light.observe(qualified(320, false));
    require(
      result.candidates_remaining == 1,
      "second mismatch count is incorrect"
    );
    result = low_light.observe(qualified(330, false));
    require(
      result.action == LockScreenAction::PasswordFallback &&
      result.reason == "candidate_limit_reached" &&
      result.candidates_remaining == 0,
      "three mismatches did not fall back to password"
    );

    LockScreenAuthSession success;
    (void)success.observe(qualified(20, false));
    result = success.observe(qualified(30, true));
    require(
      result.action == LockScreenAction::Success &&
      result.qualified_candidates == 2 &&
      success.terminal(),
      "successful match did not terminate"
    );

    LockScreenAuthSession timeout;
    result = timeout.observe(qualified(1000, true));
    require(
      result.action == LockScreenAction::PasswordFallback &&
      result.reason == "deadline_reached",
      "deadline did not take precedence"
    );

    LockScreenAuthSession cancelled;
    result = cancelled.cancel("password_started");
    require(
      result.action == LockScreenAction::Cancelled &&
      result.reason == "password_started",
      "password cancellation failed"
    );
    result = cancelled.observe(qualified(10, true));
    require(
      result.action == LockScreenAction::Cancelled,
      "cancelled session resumed"
    );

    bool invalid_policy_rejected = false;
    try {
      face_unlock::LockScreenPolicy invalid;
      invalid.maximum_candidates = 0;
      LockScreenAuthSession invalid_session(invalid);
      (void)invalid_session;
    } catch (const std::runtime_error&) {
      invalid_policy_rejected = true;
    }
    require(invalid_policy_rejected, "invalid policy was accepted");

    bool early_ack_rejected = false;
    try {
      LockScreenAuthSession no_request;
      (void)no_request.acknowledge_illumination(1);
    } catch (const std::runtime_error&) {
      early_ack_rejected = true;
    }
    require(early_ack_rejected, "unrequested illumination was acknowledged");

    bool reversed_time_rejected = false;
    try {
      LockScreenAuthSession monotonic;
      LockScreenObservation first;
      first.elapsed_ms = 20;
      (void)monotonic.observe(first);
      first.elapsed_ms = 19;
      (void)monotonic.observe(first);
    } catch (const std::runtime_error&) {
      reversed_time_rejected = true;
    }
    require(reversed_time_rejected, "reversed elapsed time was accepted");

    std::cout << "maximum_candidates: 3\n";
    std::cout << "deadline_ms: 1000\n";
    std::cout << "illumination_settle_ms: 200\n";
    std::cout << "low_quality_counts_as_attempt: false\n";
    std::cout << "password_fallback_status: ok\n";
    std::cout << "lockscreen_policy_status: ok\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "status: failed\n";
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
