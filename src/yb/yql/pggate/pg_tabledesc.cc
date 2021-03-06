//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//
//--------------------------------------------------------------------------------------------------

#include "yb/yql/pggate/pg_tabledesc.h"

#include "yb/client/table.h"

namespace yb {
namespace pggate {

PgTableDesc::PgTableDesc(std::shared_ptr<client::YBTable> pg_table) : table_(pg_table) {
  const auto& schema = pg_table->schema();
  const int num_columns = schema.num_columns();
  columns_.resize(num_columns);
  for (size_t idx = 0; idx < num_columns; idx++) {
    // Find the column descriptor.
    const auto& col = schema.Column(idx);

    // TODO(neil) Considering index columns by attr_num instead of ID.
    ColumnDesc *desc = columns_[idx].desc();
    desc->Init(idx,
               schema.ColumnId(idx),
               col.name(),
               idx < schema.num_hash_key_columns(),
               idx < schema.num_key_columns(),
               col.order() /* attr_num */,
               col.type(),
               client::YBColumnSchema::ToInternalDataType(col.type()),
               col.sorting_type());
    attr_num_map_[col.order()] = idx;
  }

  // Create virtual columns.
  column_ybctid_.Init(PgSystemAttrNum::kYBTupleId);
}

Result<PgColumn *> PgTableDesc::FindColumn(int attr_num) {
  // Find virtual columns.
  if (attr_num == static_cast<int>(PgSystemAttrNum::kYBTupleId)) {
    return &column_ybctid_;
  }

  // Find physical column.
  const auto itr = attr_num_map_.find(attr_num);
  if (itr != attr_num_map_.end()) {
    return &columns_[itr->second];
  }

  return STATUS_FORMAT(InvalidArgument, "Invalid column number $0", attr_num);
}

Status PgTableDesc::GetColumnInfo(int16_t attr_number, bool *is_primary, bool *is_hash) const {
  const auto itr = attr_num_map_.find(attr_number);
  if (itr != attr_num_map_.end()) {
    const ColumnDesc* desc = columns_[itr->second].desc();
    *is_primary = desc->is_primary();
    *is_hash = desc->is_partition();
  } else {
    *is_primary = false;
    *is_hash = false;
  }
  return Status::OK();
}

bool PgTableDesc::IsTransactional() const {
  return table_->schema().table_properties().is_transactional();
}

const std::vector<std::string>& PgTableDesc::GetPartitions() const {
  return table_->GetPartitions();
}

int PgTableDesc::GetPartitionCount() const {
  return table_->GetPartitionCount();
}

size_t PgTableDesc::FindPartitionStartIndex(const std::string& partition_key) const {
  return table_->FindPartitionStartIndex(partition_key);
}

const client::YBTableName& PgTableDesc::table_name() const {
  return table_->name();
}

const size_t PgTableDesc::num_hash_key_columns() const {
  return table_->schema().num_hash_key_columns();
}

const size_t PgTableDesc::num_key_columns() const {
  return table_->schema().num_key_columns();
}

const size_t PgTableDesc::num_columns() const {
  return table_->schema().num_columns();
}

std::unique_ptr<client::YBPgsqlReadOp> PgTableDesc::NewPgsqlSelect() {
  return table_->NewPgsqlSelect();
}

std::unique_ptr<client::YBPgsqlWriteOp> PgTableDesc::NewPgsqlInsert() {
  return table_->NewPgsqlInsert();
}

std::unique_ptr<client::YBPgsqlWriteOp> PgTableDesc::NewPgsqlUpdate() {
  return table_->NewPgsqlUpdate();
}

std::unique_ptr<client::YBPgsqlWriteOp> PgTableDesc::NewPgsqlDelete() {
  return table_->NewPgsqlDelete();
}

std::unique_ptr<client::YBPgsqlWriteOp> PgTableDesc::NewPgsqlTruncateColocated() {
  return table_->NewPgsqlTruncateColocated();
}

}  // namespace pggate
}  // namespace yb
