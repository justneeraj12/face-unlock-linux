#include "face_profile.h"
#include "template_crypto.h"

#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

template <typename Function>
bool throws_runtime_error(Function&& function) {
  try {
    function();
  } catch (const std::runtime_error&) {
    return true;
  }
  return false;
}

face_unlock::FaceEmbedding embedding(
  std::initializer_list<float> values
) {
  return {"opencv-sface-2021dec", values};
}

face_unlock::DetectionBox detection(float nose_x, float nose_y) {
  face_unlock::DetectionBox result;
  result.landmarks = {
    {40.0F, 40.0F},
    {60.0F, 40.0F},
    {nose_x, nose_y},
    {42.0F, 80.0F},
    {58.0F, 80.0F},
  };
  return result;
}

}  // namespace

int main() {
  try {
    using face_unlock::PoseSlot;

    if (face_unlock::classify_pose(detection(50.0F, 60.0F)) !=
          PoseSlot::Center ||
        face_unlock::classify_pose(detection(46.0F, 60.0F)) !=
          PoseSlot::Left ||
        face_unlock::classify_pose(detection(54.0F, 60.0F)) !=
          PoseSlot::Right ||
        face_unlock::classify_pose(detection(50.0F, 54.0F)) !=
          PoseSlot::Up ||
        face_unlock::classify_pose(detection(50.0F, 66.0F)) !=
          PoseSlot::Down) {
      throw std::runtime_error("pose classification failed");
    }

    face_unlock::FaceProfileBuilder builder(
      "opencv-sface-2021dec",
      2,
      4,
      0.9999,
      0.5
    );

    if (builder.add_sample(
          embedding({1.0F, 0.0F, 0.0F}),
          PoseSlot::Center,
          0.4
        ).reason != "quality_too_low" ||
        builder.add_sample(
          {"other-model", {1.0F, 0.0F, 0.0F}},
          PoseSlot::Center,
          0.9
        ).reason != "embedding_model_mismatch") {
      throw std::runtime_error("sample rejection failed");
    }

    const std::vector<std::pair<PoseSlot, std::vector<float>>> bases = {
      {PoseSlot::Center, {1.0F, 0.0F, 0.0F}},
      {PoseSlot::Left, {0.8F, 0.6F, 0.0F}},
      {PoseSlot::Right, {0.8F, -0.6F, 0.0F}},
      {PoseSlot::Up, {0.8F, 0.0F, 0.6F}},
      {PoseSlot::Down, {0.8F, 0.0F, -0.6F}},
    };

    for (const auto& item : bases) {
      const auto first = builder.add_sample(
        {"opencv-sface-2021dec", item.second},
        item.first,
        0.9
      );
      std::vector<float> varied = item.second;
      varied[0] -= 0.02F;
      varied[1] += 0.03F;
      varied[2] += 0.01F;
      const auto second = builder.add_sample(
        {"opencv-sface-2021dec", varied},
        item.first,
        0.9
      );
      if (!first.accepted || !second.accepted) {
        throw std::runtime_error("valid sample was rejected");
      }
    }

    if (!builder.ready() || builder.progress_percent() != 100) {
      throw std::runtime_error("profile builder did not reach ready state");
    }
    if (builder.add_sample(
          embedding({1.0F, 0.0F, 0.0F}),
          PoseSlot::Center,
          0.9
        ).reason != "duplicate_sample") {
      throw std::runtime_error("duplicate sample was not rejected");
    }

    const face_unlock::FaceProfile profile = builder.finalize();
    const face_unlock::ProfileMatch match = profile.score(
      embedding({1.0F, 0.0F, 0.0F})
    );
    if (match.pose != PoseSlot::Center || match.similarity < 0.99) {
      throw std::runtime_error("profile score failed");
    }

    const std::vector<unsigned char> serialized =
      face_unlock::serialize_face_profile(profile);
    if (serialized.size() != 140) {
      throw std::runtime_error("unexpected version-1 payload size");
    }
    const face_unlock::FaceProfile parsed =
      face_unlock::deserialize_face_profile(serialized);
    const face_unlock::ProfileMatch parsed_match = parsed.score(
      embedding({1.0F, 0.0F, 0.0F})
    );
    if (parsed.model_id != profile.model_id ||
        parsed.embedding_dim != profile.embedding_dim ||
        parsed_match.pose != match.pose ||
        std::abs(parsed_match.similarity - match.similarity) > 0.000001) {
      throw std::runtime_error("profile round trip failed");
    }

    const std::vector<unsigned char> key = face_unlock::generate_random_key();
    const face_unlock::EncryptedBlob encrypted =
      face_unlock::encrypt_template_bytes(serialized, key);
    const std::vector<unsigned char> decrypted =
      face_unlock::decrypt_template_bytes(encrypted, key);
    const face_unlock::FaceProfile decrypted_profile =
      face_unlock::deserialize_face_profile(decrypted);
    if (decrypted_profile.poses.size() != 5) {
      throw std::runtime_error("encrypted profile round trip failed");
    }

    face_unlock::EncryptedBlob tampered = encrypted;
    tampered.bytes.back() ^= 0xffU;
    if (!throws_runtime_error([&]() {
          (void)face_unlock::decrypt_template_bytes(tampered, key);
        })) {
      throw std::runtime_error("tampered encrypted profile was accepted");
    }

    for (std::size_t length = 0; length < serialized.size(); ++length) {
      const std::vector<unsigned char> truncated(
        serialized.begin(),
        serialized.begin() + static_cast<std::ptrdiff_t>(length)
      );
      if (!throws_runtime_error([&]() {
            (void)face_unlock::deserialize_face_profile(truncated);
          })) {
        throw std::runtime_error("truncated profile was accepted");
      }
    }

    std::vector<unsigned char> trailing = serialized;
    trailing.push_back(0);
    std::vector<unsigned char> bad_magic = serialized;
    bad_magic[0] ^= 0xffU;
    std::vector<unsigned char> bad_version = serialized;
    bad_version[8] = 2;
    if (!throws_runtime_error([&]() {
          (void)face_unlock::deserialize_face_profile(trailing);
        }) ||
        !throws_runtime_error([&]() {
          (void)face_unlock::deserialize_face_profile(bad_magic);
        }) ||
        !throws_runtime_error([&]() {
          (void)face_unlock::deserialize_face_profile(bad_version);
        }) ||
        !throws_runtime_error([&]() {
          (void)parsed.score({"wrong-model", {1.0F, 0.0F, 0.0F}});
        })) {
      throw std::runtime_error("malformed profile validation failed");
    }

    std::cout << "profile_format_version: "
              << profile.format_version << '\n';
    std::cout << "profile_embedding_dim: "
              << profile.embedding_dim << '\n';
    std::cout << "profile_pose_templates: "
              << profile.poses.size() << '\n';
    std::cout << "profile_payload_bytes: "
              << serialized.size() << '\n';
    std::cout << "profile_best_pose: "
              << face_unlock::pose_slot_name(match.pose) << '\n';
    std::cout << "profile_encrypted_roundtrip: ok\n";
    std::cout << "profile_validation_status: ok\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "status: failed\n";
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
