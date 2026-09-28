#pragma once

#include <string>

namespace face_unlock {

struct LockScreenPolicy {
  int maximum_candidates = 3;
  int deadline_ms = 1000;
  int illumination_settle_ms = 200;
  int low_light_luma_threshold = 50;
};

enum class LockScreenAction {
  Continue,
  RequestIllumination,
  Success,
  PasswordFallback,
  Cancelled,
};

struct LockScreenObservation {
  int elapsed_ms = 0;
  bool frame_available = false;
  bool single_face = false;
  bool quality_approved = false;
  bool low_light = false;
  bool matched = false;
};

struct LockScreenDecision {
  LockScreenAction action = LockScreenAction::Continue;
  std::string reason;
  int qualified_candidates = 0;
  int candidates_remaining = 0;
};

class LockScreenAuthSession final {
public:
  explicit LockScreenAuthSession(
    LockScreenPolicy policy = LockScreenPolicy{}
  );

  LockScreenDecision observe(const LockScreenObservation& observation);
  LockScreenDecision acknowledge_illumination(int elapsed_ms);
  LockScreenDecision cancel(const std::string& reason = "cancelled");

  const LockScreenPolicy& policy() const;
  bool terminal() const;

private:
  LockScreenDecision decision(
    LockScreenAction action,
    const std::string& reason
  ) const;
  LockScreenDecision finish(
    LockScreenAction action,
    const std::string& reason
  );

  LockScreenPolicy policy_;
  int qualified_candidates_ = 0;
  int last_elapsed_ms_ = 0;
  int illumination_ready_ms_ = 0;
  bool illumination_requested_ = false;
  bool illumination_active_ = false;
  bool terminal_ = false;
  LockScreenAction terminal_action_ = LockScreenAction::Continue;
  std::string terminal_reason_;
};

std::string lock_screen_action_name(LockScreenAction action);

}  // namespace face_unlock
