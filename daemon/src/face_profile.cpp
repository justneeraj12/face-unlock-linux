#include "face_profile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace face_unlock {
namespace {

constexpr std::array<unsigned char, 8> kProfileMagic = {
  'F', 'U', 'L', 'P', 'R', 'F', '1', 0,
};
constexpr std::size_t kPoseCount = 5;
constexpr std::size_t kMaximumModelIdSize = 128;
constexpr std::size_t kMaximumEmbeddingDimension = 4096;
constexpr std::uint32_t kMaximumSamplesPerPose = 64;

std::size_t pose_index(PoseSlot pose) {
  const auto value = static_cast<std::size_t>(pose);
  if (value >= kPoseCount) {
    throw std::runtime_error("unsupported pose slot");
  }
  return value;
}

bool valid_model_id(const std::string& model_id) {
  if (model_id.empty() || model_id.size() > kMaximumModelIdSize) {
    return false;
  }

  return std::all_of(
    model_id.begin(),
    model_id.end(),
    [](unsigned char character) {
      return
        (character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') ||
        (character >= '0' && character <= '9') ||
        character == '.' || character == '_' || character == '-';
    }
  );
}

FaceEmbedding normalized_copy(const FaceEmbedding& embedding) {
  if (embedding.model.empty() || embedding.values.empty()) {
    throw std::runtime_error("embedding is missing model or values");
  }

  FaceEmbedding normalized;
  normalized.model = embedding.model;
  normalized.values.reserve(embedding.values.size());

  double squared_norm = 0.0;
  for (const float value : embedding.values) {
    if (!std::isfinite(value)) {
      throw std::runtime_error("embedding contains non-finite values");
    }
    squared_norm += static_cast<double>(value) * value;
  }

  const double norm = std::sqrt(squared_norm);
  if (!std::isfinite(norm) || norm <= 0.0) {
    throw std::runtime_error("embedding norm must be positive");
  }

  for (const float value : embedding.values) {
    normalized.values.push_back(
      static_cast<float>(static_cast<double>(value) / norm)
    );
  }

  return normalized;
}

FaceEmbedding robust_centroid(
  const std::string& model_id,
  const std::vector<FaceEmbedding>& samples
) {
  if (samples.empty()) {
    throw std::runtime_error("cannot build centroid without samples");
  }

  const std::size_t dimension = samples.front().values.size();
  std::vector<double> mean(dimension, 0.0);

  for (const FaceEmbedding& sample : samples) {
    for (std::size_t index = 0; index < dimension; ++index) {
      mean[index] += sample.values[index];
    }
  }

  FaceEmbedding initial;
  initial.model = model_id;
  initial.values.reserve(dimension);
  for (const double value : mean) {
    initial.values.push_back(static_cast<float>(value / samples.size()));
  }
  initial = normalized_copy(initial);

  std::vector<std::pair<double, std::size_t>> ranked;
  ranked.reserve(samples.size());
  for (std::size_t index = 0; index < samples.size(); ++index) {
    ranked.emplace_back(
      SFaceEmbedder::cosine_similarity(samples[index], initial),
      index
    );
  }
  std::sort(ranked.begin(), ranked.end());

  std::size_t discard_count = 0;
  if (samples.size() >= 5) {
    discard_count = std::max<std::size_t>(
      1,
      static_cast<std::size_t>(
        std::floor(static_cast<double>(samples.size()) * 0.10)
      )
    );
  }

  std::fill(mean.begin(), mean.end(), 0.0);
  for (std::size_t rank = discard_count; rank < ranked.size(); ++rank) {
    const FaceEmbedding& sample = samples[ranked[rank].second];
    for (std::size_t index = 0; index < dimension; ++index) {
      mean[index] += sample.values[index];
    }
  }

  const std::size_t retained = ranked.size() - discard_count;
  FaceEmbedding centroid;
  centroid.model = model_id;
  centroid.values.reserve(dimension);
  for (const double value : mean) {
    centroid.values.push_back(static_cast<float>(value / retained));
  }
  return normalized_copy(centroid);
}

void append_u16(std::vector<unsigned char>& bytes, std::uint16_t value) {
  bytes.push_back(static_cast<unsigned char>(value & 0xffU));
  bytes.push_back(static_cast<unsigned char>((value >> 8U) & 0xffU));
}

void append_u32(std::vector<unsigned char>& bytes, std::uint32_t value) {
  for (unsigned int shift = 0; shift < 32; shift += 8) {
    bytes.push_back(static_cast<unsigned char>((value >> shift) & 0xffU));
  }
}

void append_float(std::vector<unsigned char>& bytes, float value) {
  static_assert(sizeof(float) == sizeof(std::uint32_t));
  static_assert(std::numeric_limits<float>::is_iec559);

  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  append_u32(bytes, bits);
}

class ByteReader final {
public:
  explicit ByteReader(const std::vector<unsigned char>& bytes)
      : bytes_(bytes) {}

  unsigned char read_u8() {
    require(1);
    return bytes_[offset_++];
  }

  std::uint16_t read_u16() {
    require(2);
    const std::uint16_t value =
      static_cast<std::uint16_t>(bytes_[offset_]) |
      static_cast<std::uint16_t>(bytes_[offset_ + 1]) << 8U;
    offset_ += 2;
    return value;
  }

  std::uint32_t read_u32() {
    require(4);
    std::uint32_t value = 0;
    for (unsigned int shift = 0; shift < 32; shift += 8) {
      value |= static_cast<std::uint32_t>(bytes_[offset_++]) << shift;
    }
    return value;
  }

  float read_float() {
    const std::uint32_t bits = read_u32();
    float value = 0.0F;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
  }

  std::string read_string(std::size_t size) {
    require(size);
    const auto begin = bytes_.begin() + static_cast<std::ptrdiff_t>(offset_);
    offset_ += size;
    return std::string(begin, begin + static_cast<std::ptrdiff_t>(size));
  }

  void expect_magic() {
    require(kProfileMagic.size());
    if (!std::equal(
          kProfileMagic.begin(),
          kProfileMagic.end(),
          bytes_.begin() + static_cast<std::ptrdiff_t>(offset_)
        )) {
      throw std::runtime_error("face profile magic mismatch");
    }
    offset_ += kProfileMagic.size();
  }

  bool finished() const {
    return offset_ == bytes_.size();
  }

private:
  void require(std::size_t size) const {
    if (size > bytes_.size() - std::min(offset_, bytes_.size())) {
      throw std::runtime_error("face profile payload is truncated");
    }
  }

  const std::vector<unsigned char>& bytes_;
  std::size_t offset_ = 0;
};

}  // namespace

std::string pose_slot_name(PoseSlot pose) {
  switch (pose) {
    case PoseSlot::Center:
      return "center";
    case PoseSlot::Left:
      return "left";
    case PoseSlot::Right:
      return "right";
    case PoseSlot::Up:
      return "up";
    case PoseSlot::Down:
      return "down";
    case PoseSlot::Unknown:
      return "unknown";
  }
  return "unknown";
}

PoseSlot classify_pose(const DetectionBox& detection) {
  if (detection.landmarks.size() != 5) {
    return PoseSlot::Unknown;
  }

  for (const cv::Point2f& point : detection.landmarks) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
      return PoseSlot::Unknown;
    }
  }

  const cv::Point2f& right_eye = detection.landmarks[0];
  const cv::Point2f& left_eye = detection.landmarks[1];
  const cv::Point2f& nose = detection.landmarks[2];
  const cv::Point2f& right_mouth = detection.landmarks[3];
  const cv::Point2f& left_mouth = detection.landmarks[4];

  const double eye_mid_x = (right_eye.x + left_eye.x) / 2.0;
  const double eye_mid_y = (right_eye.y + left_eye.y) / 2.0;
  const double mouth_mid_y = (right_mouth.y + left_mouth.y) / 2.0;
  const double eye_span = std::max(
    std::abs(static_cast<double>(left_eye.x - right_eye.x)),
    1.0
  );
  const double eye_to_mouth = std::max(mouth_mid_y - eye_mid_y, 1.0);
  const double yaw = (nose.x - eye_mid_x) / eye_span;
  const double pitch = (nose.y - eye_mid_y) / eye_to_mouth;

  if (yaw <= -0.16) {
    return PoseSlot::Left;
  }
  if (yaw >= 0.16) {
    return PoseSlot::Right;
  }
  if (pitch <= 0.40) {
    return PoseSlot::Up;
  }
  if (pitch >= 0.62) {
    return PoseSlot::Down;
  }
  return PoseSlot::Center;
}

void validate_face_profile(const FaceProfile& profile) {
  if (profile.format_version != kFaceProfileFormatVersion) {
    throw std::runtime_error("unsupported face profile version");
  }
  if (!valid_model_id(profile.model_id)) {
    throw std::runtime_error("invalid face profile model id");
  }
  if (profile.embedding_dim == 0 ||
      profile.embedding_dim > kMaximumEmbeddingDimension) {
    throw std::runtime_error("invalid face profile embedding dimension");
  }
  if (profile.poses.size() != kPoseCount) {
    throw std::runtime_error("face profile requires five pose templates");
  }

  for (std::size_t index = 0; index < kPoseCount; ++index) {
    const PoseTemplate& pose_template = profile.poses[index];
    if (pose_template.pose != static_cast<PoseSlot>(index)) {
      throw std::runtime_error("face profile poses are missing or out of order");
    }
    if (pose_template.sample_count == 0 ||
        pose_template.sample_count > kMaximumSamplesPerPose) {
      throw std::runtime_error("invalid face profile sample count");
    }
    if (pose_template.centroid.size() != profile.embedding_dim) {
      throw std::runtime_error("face profile centroid dimension mismatch");
    }

    double squared_norm = 0.0;
    for (const float value : pose_template.centroid) {
      if (!std::isfinite(value)) {
        throw std::runtime_error("face profile contains non-finite values");
      }
      squared_norm += static_cast<double>(value) * value;
    }

    const double norm = std::sqrt(squared_norm);
    if (!std::isfinite(norm) || std::abs(norm - 1.0) > 0.001) {
      throw std::runtime_error("face profile centroid is not normalized");
    }
  }
}

ProfileMatch FaceProfile::score(const FaceEmbedding& query) const {
  validate_face_profile(*this);
  if (query.model != model_id) {
    throw std::runtime_error("query model does not match face profile");
  }
  if (query.values.size() != embedding_dim) {
    throw std::runtime_error("query dimension does not match face profile");
  }

  const FaceEmbedding normalized_query = normalized_copy(query);
  ProfileMatch best;

  for (const PoseTemplate& pose_template : poses) {
    const FaceEmbedding centroid{model_id, pose_template.centroid};
    const double similarity = SFaceEmbedder::cosine_similarity(
      normalized_query,
      centroid
    );
    if (best.pose == PoseSlot::Unknown || similarity > best.similarity) {
      best.pose = pose_template.pose;
      best.similarity = similarity;
    }
  }

  return best;
}

std::vector<unsigned char> serialize_face_profile(const FaceProfile& profile) {
  validate_face_profile(profile);

  std::vector<unsigned char> bytes;
  bytes.reserve(
    20 + profile.model_id.size() +
    profile.poses.size() * (8 + profile.embedding_dim * sizeof(float))
  );
  bytes.insert(bytes.end(), kProfileMagic.begin(), kProfileMagic.end());
  append_u16(bytes, profile.format_version);
  append_u16(bytes, 0);
  append_u16(bytes, static_cast<std::uint16_t>(profile.model_id.size()));
  append_u16(bytes, static_cast<std::uint16_t>(profile.poses.size()));
  append_u32(bytes, static_cast<std::uint32_t>(profile.embedding_dim));
  bytes.insert(bytes.end(), profile.model_id.begin(), profile.model_id.end());

  for (const PoseTemplate& pose_template : profile.poses) {
    bytes.push_back(static_cast<unsigned char>(pose_template.pose));
    bytes.push_back(0);
    bytes.push_back(0);
    bytes.push_back(0);
    append_u32(bytes, pose_template.sample_count);
    for (const float value : pose_template.centroid) {
      append_float(bytes, value);
    }
  }

  return bytes;
}

FaceProfile deserialize_face_profile(const std::vector<unsigned char>& bytes) {
  ByteReader reader(bytes);
  reader.expect_magic();

  FaceProfile profile;
  profile.format_version = reader.read_u16();
  if (profile.format_version != kFaceProfileFormatVersion) {
    throw std::runtime_error("unsupported face profile version");
  }
  const std::uint16_t flags = reader.read_u16();
  const std::uint16_t model_id_size = reader.read_u16();
  const std::uint16_t pose_count = reader.read_u16();
  const std::uint32_t embedding_dim = reader.read_u32();

  if (flags != 0) {
    throw std::runtime_error("unsupported face profile flags");
  }
  if (model_id_size == 0 || model_id_size > kMaximumModelIdSize) {
    throw std::runtime_error("invalid face profile model id length");
  }
  if (pose_count != kPoseCount) {
    throw std::runtime_error("face profile requires five pose templates");
  }
  if (embedding_dim == 0 || embedding_dim > kMaximumEmbeddingDimension) {
    throw std::runtime_error("invalid face profile embedding dimension");
  }

  profile.model_id = reader.read_string(model_id_size);
  profile.embedding_dim = embedding_dim;
  profile.poses.reserve(pose_count);

  for (std::size_t index = 0; index < pose_count; ++index) {
    PoseTemplate pose_template;
    pose_template.pose = static_cast<PoseSlot>(reader.read_u8());
    if (reader.read_u8() != 0 ||
        reader.read_u8() != 0 ||
        reader.read_u8() != 0) {
      throw std::runtime_error("unsupported face profile pose flags");
    }
    pose_template.sample_count = reader.read_u32();
    pose_template.centroid.reserve(embedding_dim);
    for (std::size_t value_index = 0;
         value_index < embedding_dim;
         ++value_index) {
      pose_template.centroid.push_back(reader.read_float());
    }
    profile.poses.push_back(std::move(pose_template));
  }

  if (!reader.finished()) {
    throw std::runtime_error("face profile payload has trailing data");
  }

  validate_face_profile(profile);
  return profile;
}

FaceProfileBuilder::FaceProfileBuilder(
  std::string model_id,
  std::size_t target_per_pose,
  std::size_t maximum_per_pose,
  double duplicate_similarity,
  double minimum_quality
) :
    model_id_(std::move(model_id)),
    target_per_pose_(target_per_pose),
    maximum_per_pose_(maximum_per_pose),
    duplicate_similarity_(duplicate_similarity),
    minimum_quality_(minimum_quality) {
  if (!valid_model_id(model_id_)) {
    throw std::runtime_error("invalid face profile model id");
  }
  if (target_per_pose_ == 0) {
    throw std::runtime_error("target samples per pose must be positive");
  }
  if (maximum_per_pose_ < target_per_pose_ ||
      maximum_per_pose_ > kMaximumSamplesPerPose) {
    throw std::runtime_error("invalid maximum samples per pose");
  }
  if (!std::isfinite(duplicate_similarity_) ||
      duplicate_similarity_ < -1.0 ||
      duplicate_similarity_ > 1.0) {
    throw std::runtime_error("invalid duplicate similarity");
  }
  if (!std::isfinite(minimum_quality_) ||
      minimum_quality_ < 0.0 ||
      minimum_quality_ > 1.0) {
    throw std::runtime_error("invalid minimum quality");
  }
}

int FaceProfileBuilder::progress_percent() const {
  const std::size_t required = target_per_pose_ * kPoseCount;
  std::size_t captured = 0;
  for (const auto& pose_samples : samples_) {
    captured += std::min(pose_samples.size(), target_per_pose_);
  }
  return static_cast<int>((captured * 100 + required / 2) / required);
}

bool FaceProfileBuilder::ready() const {
  return missing_poses().empty();
}

std::vector<PoseSlot> FaceProfileBuilder::missing_poses() const {
  std::vector<PoseSlot> missing;
  for (std::size_t index = 0; index < kPoseCount; ++index) {
    if (samples_[index].size() < target_per_pose_) {
      missing.push_back(static_cast<PoseSlot>(index));
    }
  }
  return missing;
}

AddSampleResult FaceProfileBuilder::add_sample(
  const FaceEmbedding& embedding,
  PoseSlot pose,
  double quality_score
) {
  AddSampleResult result;
  result.pose = pose;
  result.progress_percent = progress_percent();

  if (pose == PoseSlot::Unknown ||
      static_cast<std::size_t>(pose) >= kPoseCount) {
    result.reason = "unsupported_pose";
    return result;
  }
  if (!std::isfinite(quality_score) ||
      quality_score < 0.0 ||
      quality_score > 1.0) {
    result.reason = "invalid_quality";
    return result;
  }
  if (quality_score < minimum_quality_) {
    result.reason = "quality_too_low";
    return result;
  }
  if (embedding.model != model_id_) {
    result.reason = "embedding_model_mismatch";
    return result;
  }

  FaceEmbedding normalized;
  try {
    normalized = normalized_copy(embedding);
  } catch (const std::runtime_error&) {
    result.reason = "invalid_embedding";
    return result;
  }

  if (!embedding_dim_.has_value()) {
    embedding_dim_ = normalized.values.size();
  } else if (normalized.values.size() != embedding_dim_.value()) {
    result.reason = "embedding_dimension_mismatch";
    return result;
  }

  auto& pose_samples = samples_[pose_index(pose)];
  if (pose_samples.size() >= maximum_per_pose_) {
    result.reason = "pose_full";
    return result;
  }

  for (const FaceEmbedding& existing : pose_samples) {
    if (SFaceEmbedder::cosine_similarity(normalized, existing) >=
        duplicate_similarity_) {
      result.reason = "duplicate_sample";
      return result;
    }
  }

  pose_samples.push_back(std::move(normalized));
  result.accepted = true;
  result.reason = "accepted";
  result.progress_percent = progress_percent();
  return result;
}

FaceProfile FaceProfileBuilder::finalize() const {
  if (!ready()) {
    std::string message = "missing enrollment poses:";
    for (const PoseSlot pose : missing_poses()) {
      message += " " + pose_slot_name(pose);
    }
    throw std::runtime_error(message);
  }
  if (!embedding_dim_.has_value()) {
    throw std::runtime_error("no enrollment embeddings captured");
  }

  FaceProfile profile;
  profile.model_id = model_id_;
  profile.embedding_dim = embedding_dim_.value();
  profile.poses.reserve(kPoseCount);

  for (std::size_t index = 0; index < kPoseCount; ++index) {
    PoseTemplate pose_template;
    pose_template.pose = static_cast<PoseSlot>(index);
    pose_template.sample_count = static_cast<std::uint32_t>(
      samples_[index].size()
    );
    pose_template.centroid = robust_centroid(
      model_id_,
      samples_[index]
    ).values;
    profile.poses.push_back(std::move(pose_template));
  }

  validate_face_profile(profile);
  return profile;
}

}  // namespace face_unlock
