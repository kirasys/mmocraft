// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: world_metadata.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_world_5fmetadata_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_world_5fmetadata_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021012 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_world_5fmetadata_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_world_5fmetadata_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_world_5fmetadata_2eproto;
namespace game {
class WorldMetadata;
struct WorldMetadataDefaultTypeInternal;
extern WorldMetadataDefaultTypeInternal _WorldMetadata_default_instance_;
}  // namespace game
PROTOBUF_NAMESPACE_OPEN
template<> ::game::WorldMetadata* Arena::CreateMaybeMessage<::game::WorldMetadata>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace game {

// ===================================================================

class WorldMetadata final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:game.WorldMetadata) */ {
 public:
  inline WorldMetadata() : WorldMetadata(nullptr) {}
  ~WorldMetadata() override;
  explicit PROTOBUF_CONSTEXPR WorldMetadata(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  WorldMetadata(const WorldMetadata& from);
  WorldMetadata(WorldMetadata&& from) noexcept
    : WorldMetadata() {
    *this = ::std::move(from);
  }

  inline WorldMetadata& operator=(const WorldMetadata& from) {
    CopyFrom(from);
    return *this;
  }
  inline WorldMetadata& operator=(WorldMetadata&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const WorldMetadata& default_instance() {
    return *internal_default_instance();
  }
  static inline const WorldMetadata* internal_default_instance() {
    return reinterpret_cast<const WorldMetadata*>(
               &_WorldMetadata_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(WorldMetadata& a, WorldMetadata& b) {
    a.Swap(&b);
  }
  inline void Swap(WorldMetadata* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(WorldMetadata* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  WorldMetadata* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<WorldMetadata>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const WorldMetadata& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const WorldMetadata& from) {
    WorldMetadata::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(WorldMetadata* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "game.WorldMetadata";
  }
  protected:
  explicit WorldMetadata(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kFormatVersionFieldNumber = 1,
    kWidthFieldNumber = 2,
    kHeightFieldNumber = 3,
    kLengthFieldNumber = 4,
    kVolumeFieldNumber = 5,
    kSpawnXFieldNumber = 6,
    kSpawnYFieldNumber = 7,
    kSpawnZFieldNumber = 8,
    kSpawnYawFieldNumber = 9,
    kSpawnPitchFieldNumber = 10,
    kCreatedAtFieldNumber = 11,
  };
  // int32 format_version = 1;
  void clear_format_version();
  int32_t format_version() const;
  void set_format_version(int32_t value);
  private:
  int32_t _internal_format_version() const;
  void _internal_set_format_version(int32_t value);
  public:

  // int32 width = 2;
  void clear_width();
  int32_t width() const;
  void set_width(int32_t value);
  private:
  int32_t _internal_width() const;
  void _internal_set_width(int32_t value);
  public:

  // int32 height = 3;
  void clear_height();
  int32_t height() const;
  void set_height(int32_t value);
  private:
  int32_t _internal_height() const;
  void _internal_set_height(int32_t value);
  public:

  // int32 length = 4;
  void clear_length();
  int32_t length() const;
  void set_length(int32_t value);
  private:
  int32_t _internal_length() const;
  void _internal_set_length(int32_t value);
  public:

  // int32 volume = 5;
  void clear_volume();
  int32_t volume() const;
  void set_volume(int32_t value);
  private:
  int32_t _internal_volume() const;
  void _internal_set_volume(int32_t value);
  public:

  // int32 spawn_x = 6;
  void clear_spawn_x();
  int32_t spawn_x() const;
  void set_spawn_x(int32_t value);
  private:
  int32_t _internal_spawn_x() const;
  void _internal_set_spawn_x(int32_t value);
  public:

  // int32 spawn_y = 7;
  void clear_spawn_y();
  int32_t spawn_y() const;
  void set_spawn_y(int32_t value);
  private:
  int32_t _internal_spawn_y() const;
  void _internal_set_spawn_y(int32_t value);
  public:

  // int32 spawn_z = 8;
  void clear_spawn_z();
  int32_t spawn_z() const;
  void set_spawn_z(int32_t value);
  private:
  int32_t _internal_spawn_z() const;
  void _internal_set_spawn_z(int32_t value);
  public:

  // int32 spawn_yaw = 9;
  void clear_spawn_yaw();
  int32_t spawn_yaw() const;
  void set_spawn_yaw(int32_t value);
  private:
  int32_t _internal_spawn_yaw() const;
  void _internal_set_spawn_yaw(int32_t value);
  public:

  // int32 spawn_pitch = 10;
  void clear_spawn_pitch();
  int32_t spawn_pitch() const;
  void set_spawn_pitch(int32_t value);
  private:
  int32_t _internal_spawn_pitch() const;
  void _internal_set_spawn_pitch(int32_t value);
  public:

  // uint64 created_at = 11;
  void clear_created_at();
  uint64_t created_at() const;
  void set_created_at(uint64_t value);
  private:
  uint64_t _internal_created_at() const;
  void _internal_set_created_at(uint64_t value);
  public:

  // @@protoc_insertion_point(class_scope:game.WorldMetadata)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    int32_t format_version_;
    int32_t width_;
    int32_t height_;
    int32_t length_;
    int32_t volume_;
    int32_t spawn_x_;
    int32_t spawn_y_;
    int32_t spawn_z_;
    int32_t spawn_yaw_;
    int32_t spawn_pitch_;
    uint64_t created_at_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_world_5fmetadata_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// WorldMetadata

// int32 format_version = 1;
inline void WorldMetadata::clear_format_version() {
  _impl_.format_version_ = 0;
}
inline int32_t WorldMetadata::_internal_format_version() const {
  return _impl_.format_version_;
}
inline int32_t WorldMetadata::format_version() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.format_version)
  return _internal_format_version();
}
inline void WorldMetadata::_internal_set_format_version(int32_t value) {
  
  _impl_.format_version_ = value;
}
inline void WorldMetadata::set_format_version(int32_t value) {
  _internal_set_format_version(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.format_version)
}

// int32 width = 2;
inline void WorldMetadata::clear_width() {
  _impl_.width_ = 0;
}
inline int32_t WorldMetadata::_internal_width() const {
  return _impl_.width_;
}
inline int32_t WorldMetadata::width() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.width)
  return _internal_width();
}
inline void WorldMetadata::_internal_set_width(int32_t value) {
  
  _impl_.width_ = value;
}
inline void WorldMetadata::set_width(int32_t value) {
  _internal_set_width(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.width)
}

// int32 height = 3;
inline void WorldMetadata::clear_height() {
  _impl_.height_ = 0;
}
inline int32_t WorldMetadata::_internal_height() const {
  return _impl_.height_;
}
inline int32_t WorldMetadata::height() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.height)
  return _internal_height();
}
inline void WorldMetadata::_internal_set_height(int32_t value) {
  
  _impl_.height_ = value;
}
inline void WorldMetadata::set_height(int32_t value) {
  _internal_set_height(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.height)
}

// int32 length = 4;
inline void WorldMetadata::clear_length() {
  _impl_.length_ = 0;
}
inline int32_t WorldMetadata::_internal_length() const {
  return _impl_.length_;
}
inline int32_t WorldMetadata::length() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.length)
  return _internal_length();
}
inline void WorldMetadata::_internal_set_length(int32_t value) {
  
  _impl_.length_ = value;
}
inline void WorldMetadata::set_length(int32_t value) {
  _internal_set_length(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.length)
}

// int32 volume = 5;
inline void WorldMetadata::clear_volume() {
  _impl_.volume_ = 0;
}
inline int32_t WorldMetadata::_internal_volume() const {
  return _impl_.volume_;
}
inline int32_t WorldMetadata::volume() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.volume)
  return _internal_volume();
}
inline void WorldMetadata::_internal_set_volume(int32_t value) {
  
  _impl_.volume_ = value;
}
inline void WorldMetadata::set_volume(int32_t value) {
  _internal_set_volume(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.volume)
}

// int32 spawn_x = 6;
inline void WorldMetadata::clear_spawn_x() {
  _impl_.spawn_x_ = 0;
}
inline int32_t WorldMetadata::_internal_spawn_x() const {
  return _impl_.spawn_x_;
}
inline int32_t WorldMetadata::spawn_x() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.spawn_x)
  return _internal_spawn_x();
}
inline void WorldMetadata::_internal_set_spawn_x(int32_t value) {
  
  _impl_.spawn_x_ = value;
}
inline void WorldMetadata::set_spawn_x(int32_t value) {
  _internal_set_spawn_x(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.spawn_x)
}

// int32 spawn_y = 7;
inline void WorldMetadata::clear_spawn_y() {
  _impl_.spawn_y_ = 0;
}
inline int32_t WorldMetadata::_internal_spawn_y() const {
  return _impl_.spawn_y_;
}
inline int32_t WorldMetadata::spawn_y() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.spawn_y)
  return _internal_spawn_y();
}
inline void WorldMetadata::_internal_set_spawn_y(int32_t value) {
  
  _impl_.spawn_y_ = value;
}
inline void WorldMetadata::set_spawn_y(int32_t value) {
  _internal_set_spawn_y(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.spawn_y)
}

// int32 spawn_z = 8;
inline void WorldMetadata::clear_spawn_z() {
  _impl_.spawn_z_ = 0;
}
inline int32_t WorldMetadata::_internal_spawn_z() const {
  return _impl_.spawn_z_;
}
inline int32_t WorldMetadata::spawn_z() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.spawn_z)
  return _internal_spawn_z();
}
inline void WorldMetadata::_internal_set_spawn_z(int32_t value) {
  
  _impl_.spawn_z_ = value;
}
inline void WorldMetadata::set_spawn_z(int32_t value) {
  _internal_set_spawn_z(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.spawn_z)
}

// int32 spawn_yaw = 9;
inline void WorldMetadata::clear_spawn_yaw() {
  _impl_.spawn_yaw_ = 0;
}
inline int32_t WorldMetadata::_internal_spawn_yaw() const {
  return _impl_.spawn_yaw_;
}
inline int32_t WorldMetadata::spawn_yaw() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.spawn_yaw)
  return _internal_spawn_yaw();
}
inline void WorldMetadata::_internal_set_spawn_yaw(int32_t value) {
  
  _impl_.spawn_yaw_ = value;
}
inline void WorldMetadata::set_spawn_yaw(int32_t value) {
  _internal_set_spawn_yaw(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.spawn_yaw)
}

// int32 spawn_pitch = 10;
inline void WorldMetadata::clear_spawn_pitch() {
  _impl_.spawn_pitch_ = 0;
}
inline int32_t WorldMetadata::_internal_spawn_pitch() const {
  return _impl_.spawn_pitch_;
}
inline int32_t WorldMetadata::spawn_pitch() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.spawn_pitch)
  return _internal_spawn_pitch();
}
inline void WorldMetadata::_internal_set_spawn_pitch(int32_t value) {
  
  _impl_.spawn_pitch_ = value;
}
inline void WorldMetadata::set_spawn_pitch(int32_t value) {
  _internal_set_spawn_pitch(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.spawn_pitch)
}

// uint64 created_at = 11;
inline void WorldMetadata::clear_created_at() {
  _impl_.created_at_ = uint64_t{0u};
}
inline uint64_t WorldMetadata::_internal_created_at() const {
  return _impl_.created_at_;
}
inline uint64_t WorldMetadata::created_at() const {
  // @@protoc_insertion_point(field_get:game.WorldMetadata.created_at)
  return _internal_created_at();
}
inline void WorldMetadata::_internal_set_created_at(uint64_t value) {
  
  _impl_.created_at_ = value;
}
inline void WorldMetadata::set_created_at(uint64_t value) {
  _internal_set_created_at(value);
  // @@protoc_insertion_point(field_set:game.WorldMetadata.created_at)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace game

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_world_5fmetadata_2eproto