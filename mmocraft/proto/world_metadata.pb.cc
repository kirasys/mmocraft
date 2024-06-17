// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: world_metadata.proto

#include "world_metadata.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace game {
PROTOBUF_CONSTEXPR WorldMetadata::WorldMetadata(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.format_version_)*/0
  , /*decltype(_impl_.width_)*/0
  , /*decltype(_impl_.height_)*/0
  , /*decltype(_impl_.length_)*/0
  , /*decltype(_impl_.volume_)*/0
  , /*decltype(_impl_.spawn_x_)*/0
  , /*decltype(_impl_.spawn_y_)*/0
  , /*decltype(_impl_.spawn_z_)*/0
  , /*decltype(_impl_.spawn_yaw_)*/0
  , /*decltype(_impl_.spawn_pitch_)*/0
  , /*decltype(_impl_.created_at_)*/uint64_t{0u}
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct WorldMetadataDefaultTypeInternal {
  PROTOBUF_CONSTEXPR WorldMetadataDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~WorldMetadataDefaultTypeInternal() {}
  union {
    WorldMetadata _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 WorldMetadataDefaultTypeInternal _WorldMetadata_default_instance_;
}  // namespace game
static ::_pb::Metadata file_level_metadata_world_5fmetadata_2eproto[1];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_world_5fmetadata_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_world_5fmetadata_2eproto = nullptr;

const uint32_t TableStruct_world_5fmetadata_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.format_version_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.width_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.height_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.length_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.volume_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.spawn_x_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.spawn_y_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.spawn_z_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.spawn_yaw_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.spawn_pitch_),
  PROTOBUF_FIELD_OFFSET(::game::WorldMetadata, _impl_.created_at_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::game::WorldMetadata)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::game::_WorldMetadata_default_instance_._instance,
};

const char descriptor_table_protodef_world_5fmetadata_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\024world_metadata.proto\022\004game\"\325\001\n\rWorldMe"
  "tadata\022\026\n\016format_version\030\001 \001(\005\022\r\n\005width\030"
  "\002 \001(\005\022\016\n\006height\030\003 \001(\005\022\016\n\006length\030\004 \001(\005\022\016\n"
  "\006volume\030\005 \001(\005\022\017\n\007spawn_x\030\006 \001(\005\022\017\n\007spawn_"
  "y\030\007 \001(\005\022\017\n\007spawn_z\030\010 \001(\005\022\021\n\tspawn_yaw\030\t "
  "\001(\005\022\023\n\013spawn_pitch\030\n \001(\005\022\022\n\ncreated_at\030\013"
  " \001(\004b\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_world_5fmetadata_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_world_5fmetadata_2eproto = {
    false, false, 252, descriptor_table_protodef_world_5fmetadata_2eproto,
    "world_metadata.proto",
    &descriptor_table_world_5fmetadata_2eproto_once, nullptr, 0, 1,
    schemas, file_default_instances, TableStruct_world_5fmetadata_2eproto::offsets,
    file_level_metadata_world_5fmetadata_2eproto, file_level_enum_descriptors_world_5fmetadata_2eproto,
    file_level_service_descriptors_world_5fmetadata_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_world_5fmetadata_2eproto_getter() {
  return &descriptor_table_world_5fmetadata_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_world_5fmetadata_2eproto(&descriptor_table_world_5fmetadata_2eproto);
namespace game {

// ===================================================================

class WorldMetadata::_Internal {
 public:
};

WorldMetadata::WorldMetadata(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:game.WorldMetadata)
}
WorldMetadata::WorldMetadata(const WorldMetadata& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  WorldMetadata* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.format_version_){}
    , decltype(_impl_.width_){}
    , decltype(_impl_.height_){}
    , decltype(_impl_.length_){}
    , decltype(_impl_.volume_){}
    , decltype(_impl_.spawn_x_){}
    , decltype(_impl_.spawn_y_){}
    , decltype(_impl_.spawn_z_){}
    , decltype(_impl_.spawn_yaw_){}
    , decltype(_impl_.spawn_pitch_){}
    , decltype(_impl_.created_at_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::memcpy(&_impl_.format_version_, &from._impl_.format_version_,
    static_cast<size_t>(reinterpret_cast<char*>(&_impl_.created_at_) -
    reinterpret_cast<char*>(&_impl_.format_version_)) + sizeof(_impl_.created_at_));
  // @@protoc_insertion_point(copy_constructor:game.WorldMetadata)
}

inline void WorldMetadata::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.format_version_){0}
    , decltype(_impl_.width_){0}
    , decltype(_impl_.height_){0}
    , decltype(_impl_.length_){0}
    , decltype(_impl_.volume_){0}
    , decltype(_impl_.spawn_x_){0}
    , decltype(_impl_.spawn_y_){0}
    , decltype(_impl_.spawn_z_){0}
    , decltype(_impl_.spawn_yaw_){0}
    , decltype(_impl_.spawn_pitch_){0}
    , decltype(_impl_.created_at_){uint64_t{0u}}
    , /*decltype(_impl_._cached_size_)*/{}
  };
}

WorldMetadata::~WorldMetadata() {
  // @@protoc_insertion_point(destructor:game.WorldMetadata)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void WorldMetadata::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void WorldMetadata::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void WorldMetadata::Clear() {
// @@protoc_insertion_point(message_clear_start:game.WorldMetadata)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  ::memset(&_impl_.format_version_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&_impl_.created_at_) -
      reinterpret_cast<char*>(&_impl_.format_version_)) + sizeof(_impl_.created_at_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* WorldMetadata::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // int32 format_version = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8)) {
          _impl_.format_version_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 width = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          _impl_.width_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 height = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 24)) {
          _impl_.height_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 length = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 32)) {
          _impl_.length_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 volume = 5;
      case 5:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 40)) {
          _impl_.volume_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 spawn_x = 6;
      case 6:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 48)) {
          _impl_.spawn_x_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 spawn_y = 7;
      case 7:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 56)) {
          _impl_.spawn_y_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 spawn_z = 8;
      case 8:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 64)) {
          _impl_.spawn_z_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 spawn_yaw = 9;
      case 9:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 72)) {
          _impl_.spawn_yaw_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 spawn_pitch = 10;
      case 10:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 80)) {
          _impl_.spawn_pitch_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // uint64 created_at = 11;
      case 11:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 88)) {
          _impl_.created_at_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* WorldMetadata::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:game.WorldMetadata)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // int32 format_version = 1;
  if (this->_internal_format_version() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(1, this->_internal_format_version(), target);
  }

  // int32 width = 2;
  if (this->_internal_width() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(2, this->_internal_width(), target);
  }

  // int32 height = 3;
  if (this->_internal_height() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(3, this->_internal_height(), target);
  }

  // int32 length = 4;
  if (this->_internal_length() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(4, this->_internal_length(), target);
  }

  // int32 volume = 5;
  if (this->_internal_volume() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(5, this->_internal_volume(), target);
  }

  // int32 spawn_x = 6;
  if (this->_internal_spawn_x() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(6, this->_internal_spawn_x(), target);
  }

  // int32 spawn_y = 7;
  if (this->_internal_spawn_y() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(7, this->_internal_spawn_y(), target);
  }

  // int32 spawn_z = 8;
  if (this->_internal_spawn_z() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(8, this->_internal_spawn_z(), target);
  }

  // int32 spawn_yaw = 9;
  if (this->_internal_spawn_yaw() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(9, this->_internal_spawn_yaw(), target);
  }

  // int32 spawn_pitch = 10;
  if (this->_internal_spawn_pitch() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(10, this->_internal_spawn_pitch(), target);
  }

  // uint64 created_at = 11;
  if (this->_internal_created_at() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt64ToArray(11, this->_internal_created_at(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:game.WorldMetadata)
  return target;
}

size_t WorldMetadata::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:game.WorldMetadata)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // int32 format_version = 1;
  if (this->_internal_format_version() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_format_version());
  }

  // int32 width = 2;
  if (this->_internal_width() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_width());
  }

  // int32 height = 3;
  if (this->_internal_height() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_height());
  }

  // int32 length = 4;
  if (this->_internal_length() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_length());
  }

  // int32 volume = 5;
  if (this->_internal_volume() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_volume());
  }

  // int32 spawn_x = 6;
  if (this->_internal_spawn_x() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_spawn_x());
  }

  // int32 spawn_y = 7;
  if (this->_internal_spawn_y() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_spawn_y());
  }

  // int32 spawn_z = 8;
  if (this->_internal_spawn_z() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_spawn_z());
  }

  // int32 spawn_yaw = 9;
  if (this->_internal_spawn_yaw() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_spawn_yaw());
  }

  // int32 spawn_pitch = 10;
  if (this->_internal_spawn_pitch() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_spawn_pitch());
  }

  // uint64 created_at = 11;
  if (this->_internal_created_at() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_created_at());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData WorldMetadata::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    WorldMetadata::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*WorldMetadata::GetClassData() const { return &_class_data_; }


void WorldMetadata::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<WorldMetadata*>(&to_msg);
  auto& from = static_cast<const WorldMetadata&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:game.WorldMetadata)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_format_version() != 0) {
    _this->_internal_set_format_version(from._internal_format_version());
  }
  if (from._internal_width() != 0) {
    _this->_internal_set_width(from._internal_width());
  }
  if (from._internal_height() != 0) {
    _this->_internal_set_height(from._internal_height());
  }
  if (from._internal_length() != 0) {
    _this->_internal_set_length(from._internal_length());
  }
  if (from._internal_volume() != 0) {
    _this->_internal_set_volume(from._internal_volume());
  }
  if (from._internal_spawn_x() != 0) {
    _this->_internal_set_spawn_x(from._internal_spawn_x());
  }
  if (from._internal_spawn_y() != 0) {
    _this->_internal_set_spawn_y(from._internal_spawn_y());
  }
  if (from._internal_spawn_z() != 0) {
    _this->_internal_set_spawn_z(from._internal_spawn_z());
  }
  if (from._internal_spawn_yaw() != 0) {
    _this->_internal_set_spawn_yaw(from._internal_spawn_yaw());
  }
  if (from._internal_spawn_pitch() != 0) {
    _this->_internal_set_spawn_pitch(from._internal_spawn_pitch());
  }
  if (from._internal_created_at() != 0) {
    _this->_internal_set_created_at(from._internal_created_at());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void WorldMetadata::CopyFrom(const WorldMetadata& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:game.WorldMetadata)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool WorldMetadata::IsInitialized() const {
  return true;
}

void WorldMetadata::InternalSwap(WorldMetadata* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(WorldMetadata, _impl_.created_at_)
      + sizeof(WorldMetadata::_impl_.created_at_)
      - PROTOBUF_FIELD_OFFSET(WorldMetadata, _impl_.format_version_)>(
          reinterpret_cast<char*>(&_impl_.format_version_),
          reinterpret_cast<char*>(&other->_impl_.format_version_));
}

::PROTOBUF_NAMESPACE_ID::Metadata WorldMetadata::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_world_5fmetadata_2eproto_getter, &descriptor_table_world_5fmetadata_2eproto_once,
      file_level_metadata_world_5fmetadata_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace game
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::game::WorldMetadata*
Arena::CreateMaybeMessage< ::game::WorldMetadata >(Arena* arena) {
  return Arena::CreateMessageInternal< ::game::WorldMetadata >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>