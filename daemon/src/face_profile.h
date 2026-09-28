#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "recognizer.h"

namespace face_unlock {

constexpr std::uint16_t kFaceProfileFormatVersion = 1;

enum class PoseSlot : std::uint8_t {
  Center = 0,
  Left = 1,
  Right = 2,
  Up = 3,
  Down = 4,
  Unknown = 255,
};

struct AddSampleResult {
  bool accepted = false;
  std::string reason;
  PoseSlot pose = PoseSlot::Unknown;
  int progress_percent = 0;
};

struct PoseTemplate {
  PoseSlot pose = PoseSlot::Unknown;
  std::uint32_t sample_count = 0;
  std::vector<float> centroid;
};

struct ProfileMatch {
  PoseSlot pose = PoseSlot::Unknown;
  double similarity = -1.0;
};

struct FaceProfile {
  std::uint16_t format_version = kFaceProfileFormatVersion;
  std::string model_id;
  std::size_t embedding_dim = 0;
  std::vector<PoseTemplate> poses;

  ProfileMatch score(const FaceEmbedding& query) const;
};

std::string pose_slot_name(PoseSlot pose);
PoseSlot classify_pose(const DetectionBox& detection);

void validate_face_profile(const FaceProfile& profile);
std::vector<unsigned char> serialize_face_profile(const FaceProfile& profile);
FaceProfile deserialize_face_profile(const std::vector<unsigned char>& bytes);

class FaceProfileBuilder final {
public:
  explicit FaceProfileBuilder(
    std::string model_id,
    std::size_t target_per_pose = 6,
    std::size_t maximum_per_pose = 12,
    double duplicate_similarity = 0.9999,
    double minimum_quality = 0.5
  );

  AddSampleResult add_sample(
    const FaceEmbedding& embedding,
    PoseSlot pose,
    double quality_score
  );

  int progress_percent() const;
  bool ready() const;
  std::vector<PoseSlot> missing_poses() const;
  FaceProfile finalize() const;

private:
  std::string model_id_;
  std::size_t target_per_pose_;
  std::size_t maximum_per_pose_;
  double duplicate_similarity_;
  double minimum_quality_;
  std::optional<std::size_t> embedding_dim_;
  std::array<std::vector<FaceEmbedding>, 5> samples_;
};

}  // namespace face_unlock
