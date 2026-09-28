#include "lockscreen_auth.h"

#include <stdexcept>
#include <string>

namespace face_unlock {

std::string lock_screen_action_name(LockScreenAction action) {
  switch (action) {
    case LockScreenAction::Continue:
      return "continue";
    case LockScreenAction::RequestIllumination:
      return "request_illumination";
    case LockScreenAction::Success:
      return "success";
    case LockScreenAction::PasswordFallback:
      return "password_fallback";
    case LockScreenAction::Cancelled:
      return "cancelled";
  }
  return "cancelled";
}

LockScreenAuthSession::LockScreenAuthSession(LockScreenPolicy policy)
    : policy_(policy) {
  if (policy_.maximum_candidates < 1 ||
      policy_.maximum_candidates > 10) {
    throw std::runtime_error("invalid maximum lock-screen candidates");
  }
  if (policy_.deadline_ms < 100 || policy_.deadline_ms > 5000) {
    throw std::runtime_error("invalid lock-screen deadline");
  }
  if (policy_.illumination_settle_ms < 0 ||
      policy_.illumination_settle_ms > policy_.deadline_ms) {
    throw std::runtime_error("invalid illumination settle time");
  }
  if (policy_.low_light_luma_threshold < 0 ||
      policy_.low_light_luma_threshold > 255) {
    throw std::runtime_error("invalid low-light threshold");
  }
}

LockScreenDecision LockScreenAuthSession::decision(
  LockScreenAction action,
  const std::string& reason
) const {
  LockScreenDecision result;
  result.action = action;
  result.reason = reason;
  result.qualified_candidates = qualified_candidates_;
  result.candidates_remaining =
    policy_.maximum_candidates - qualified_candidates_;
  return result;
}

LockScreenDecision LockScreenAuthSession::finish(
  LockScreenAction action,
  const std::string& reason
) {
  terminal_ = true;
  terminal_action_ = action;
  terminal_reason_ = reason;
  return decision(action, reason);
}

LockScreenDecision LockScreenAuthSession::observe(
  const LockScreenObservation& observation
) {
  if (terminal_) {
    return decision(terminal_action_, terminal_reason_);
  }
  if (observation.elapsed_ms < last_elapsed_ms_) {
    throw std::runtime_error("lock-screen elapsed time moved backwards");
  }
  last_elapsed_ms_ = observation.elapsed_ms;

  if (observation.elapsed_ms >= policy_.deadline_ms) {
    return finish(
      LockScreenAction::PasswordFallback,
      "deadline_reached"
    );
  }

  if (!illumination_active_ && illumination_requested_) {
    return decision(
      LockScreenAction::Continue,
      "awaiting_illumination"
    );
  }

  if (observation.low_light && !illumination_active_) {
    illumination_requested_ = true;
    return decision(
      LockScreenAction::RequestIllumination,
      "low_light"
    );
  }

  if (illumination_active_ &&
      observation.elapsed_ms < illumination_ready_ms_) {
    return decision(
      LockScreenAction::Continue,
      "illumination_settling"
    );
  }

  if (!observation.frame_available) {
    return decision(LockScreenAction::Continue, "awaiting_frame");
  }
  if (!observation.single_face) {
    return decision(LockScreenAction::Continue, "single_face_required");
  }
  if (!observation.quality_approved) {
    return decision(LockScreenAction::Continue, "quality_rejected");
  }

  ++qualified_candidates_;

  if (observation.matched) {
    return finish(LockScreenAction::Success, "matched");
  }
  if (qualified_candidates_ >= policy_.maximum_candidates) {
    return finish(
      LockScreenAction::PasswordFallback,
      "candidate_limit_reached"
    );
  }

  return decision(LockScreenAction::Continue, "candidate_not_matched");
}

LockScreenDecision LockScreenAuthSession::acknowledge_illumination(
  int elapsed_ms
) {
  if (terminal_) {
    return decision(terminal_action_, terminal_reason_);
  }
  if (!illumination_requested_) {
    throw std::runtime_error("illumination was not requested");
  }
  if (elapsed_ms < last_elapsed_ms_) {
    throw std::runtime_error("lock-screen elapsed time moved backwards");
  }
  if (elapsed_ms >= policy_.deadline_ms) {
    last_elapsed_ms_ = elapsed_ms;
    return finish(
      LockScreenAction::PasswordFallback,
      "deadline_reached"
    );
  }

  last_elapsed_ms_ = elapsed_ms;
  if (illumination_active_) {
    return decision(
      LockScreenAction::Continue,
      "illumination_already_acknowledged"
    );
  }

  illumination_active_ = true;
  illumination_ready_ms_ =
    elapsed_ms + policy_.illumination_settle_ms;
  return decision(
    LockScreenAction::Continue,
    "illumination_acknowledged"
  );
}

LockScreenDecision LockScreenAuthSession::cancel(
  const std::string& reason
) {
  if (terminal_) {
    return decision(terminal_action_, terminal_reason_);
  }
  return finish(
    LockScreenAction::Cancelled,
    reason.empty() ? "cancelled" : reason
  );
}

const LockScreenPolicy& LockScreenAuthSession::policy() const {
  return policy_;
}

bool LockScreenAuthSession::terminal() const {
  return terminal_;
}

}  // namespace face_unlock
