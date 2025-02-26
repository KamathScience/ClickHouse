#pragma once

#include <Core/Field.h>
#include <Disks/IDisk.h>
#include <IO/WriteBuffer.h>
#include <Storages/KeyDescription.h>
#include <Storages/MergeTree/IPartMetadataManager.h>
#include <Storages/MergeTree/PartMetadataManagerOrdinary.h>
#include <base/types.h>

namespace DB
{

class Block;
class MergeTreeData;
struct FormatSettings;
struct MergeTreeDataPartChecksums;
struct StorageInMemoryMetadata;
class IDataPartStorage;

using StorageMetadataPtr = std::shared_ptr<const StorageInMemoryMetadata>;
using MutableDataPartStoragePtr = std::shared_ptr<IDataPartStorage>;

/// This class represents a partition value of a single part and encapsulates its loading/storing logic.
struct MergeTreePartition
{
    Row value;

public:
    MergeTreePartition() = default;

    explicit MergeTreePartition(Row value_) : value(std::move(value_)) {}

    /// For month-based partitioning.
    explicit MergeTreePartition(UInt32 yyyymm) : value(1, yyyymm) {}

    String getID(const MergeTreeData & storage) const;
    String getID(const Block & partition_key_sample) const;

    static std::optional<Row> tryParseValueFromID(const String & partition_id, const Block & partition_key_sample);

    void serializeText(const MergeTreeData & storage, WriteBuffer & out, const FormatSettings & format_settings) const;

    void load(const MergeTreeData & storage, const PartMetadataManagerPtr & manager);

    /// Store functions return write buffer with written but not finalized data.
    /// User must call finish() for returned object.
    [[nodiscard]] std::unique_ptr<WriteBufferFromFileBase> store(const MergeTreeData & storage, IDataPartStorage & data_part_storage, MergeTreeDataPartChecksums & checksums) const;
    [[nodiscard]] std::unique_ptr<WriteBufferFromFileBase> store(const Block & partition_key_sample, IDataPartStorage & data_part_storage, MergeTreeDataPartChecksums & checksums, const WriteSettings & settings) const;

    void assign(const MergeTreePartition & other) { value = other.value; }

    void create(const StorageMetadataPtr & metadata_snapshot, Block block, size_t row, ContextPtr context);

    /// Copy of MergeTreePartition::create, but also validates if min max partition keys are equal. If they are different,
    /// it means the partition can't be created because the data doesn't belong to the same partition.
    void createAndValidateMinMaxPartitionIds(
        const StorageMetadataPtr & metadata_snapshot, Block block_with_min_max_partition_ids, ContextPtr context);

    static void appendFiles(const MergeTreeData & storage, Strings & files);

    /// Adjust partition key and execute its expression on block. Return sample block according to used expression.
    static NamesAndTypesList executePartitionByExpression(const StorageMetadataPtr & metadata_snapshot, Block & block, ContextPtr context);

    /// Make a modified partition key with substitution from modulo to moduloLegacy. Used in paritionPruner.
    static KeyDescription adjustPartitionKey(const StorageMetadataPtr & metadata_snapshot, ContextPtr context);
};

}
