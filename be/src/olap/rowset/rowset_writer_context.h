// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <gen_cpp/olap_file.pb.h>

#include "io/fs/file_system.h"
#include "olap/olap_define.h"
#include "olap/partial_update_info.h"
#include "olap/tablet.h"
#include "olap/tablet_schema.h"

namespace doris {

class RowsetWriterContextBuilder;
using RowsetWriterContextBuilderSharedPtr = std::shared_ptr<RowsetWriterContextBuilder>;
class DataDir;
class Tablet;
class FileWriterCreator;
class SegmentCollector;
namespace vectorized::schema_util {
class LocalSchemaChangeRecorder;
}

struct RowsetWriterContext {
    RowsetWriterContext() : schema_lock(new std::mutex) {
        load_id.set_hi(0);
        load_id.set_lo(0);
    }

    RowsetId rowset_id;
    int64_t tablet_id {0};
    int64_t tablet_schema_hash {0};
    int64_t index_id {0};
    int64_t partition_id {0};
    RowsetTypePB rowset_type {BETA_ROWSET};
    io::FileSystemSPtr fs;
    std::string rowset_dir;
    TabletSchemaSPtr tablet_schema;
    TabletSchemaSPtr original_tablet_schema;
    // PREPARED/COMMITTED for pending rowset
    // VISIBLE for non-pending rowset
    RowsetStatePB rowset_state {PREPARED};
    // properties for non-pending rowset
    Version version {0, 0};

    // properties for pending rowset
    int64_t txn_id {0};
    int64_t txn_expiration {0}; // For cloud mode
    PUniqueId load_id;
    TabletUid tablet_uid {0, 0};
    // indicate whether the data among segments is overlapping.
    // default is OVERLAP_UNKNOWN.
    SegmentsOverlapPB segments_overlap {OVERLAP_UNKNOWN};
    // segment file use uint32 to represent row number, therefore the maximum is UINT32_MAX.
    // the default is set to INT32_MAX to avoid overflow issue when casting from uint32_t to int.
    // test cases can change this value to control flush timing
    uint32_t max_rows_per_segment = INT32_MAX;
    // not owned, point to the data dir of this rowset
    // for checking disk capacity when write data to disk.
    // ATTN: not support for RowsetConvertor.
    // (because it hard to refactor, and RowsetConvertor will be deprecated in future)
    DataDir* data_dir = nullptr;

    int64_t newest_write_timestamp = -1;
    bool enable_unique_key_merge_on_write = false;
    std::set<int32_t> skip_inverted_index;
    DataWriteType write_type = DataWriteType::TYPE_DEFAULT;
    BaseTabletSPtr tablet = nullptr;

    std::shared_ptr<MowContext> mow_context;
    std::shared_ptr<FileWriterCreator> file_writer_creator;
    std::shared_ptr<SegmentCollector> segment_collector;

    /// begin file cache opts
    bool write_file_cache = false;
    bool is_hot_data = false;
    int64_t file_cache_ttl_sec = 0;
    /// end file cache opts

    // segcompaction for this RowsetWriter, disable it for some transient writers
    bool enable_segcompaction = true;

    std::shared_ptr<PartialUpdateInfo> partial_update_info;

    bool is_transient_rowset_writer = false;
    // In semi-structure senario tablet_schema will be updated concurrently,
    // this lock need to be held when update.Use shared_ptr to avoid delete copy contructor
    std::shared_ptr<std::mutex> schema_lock;
};

} // namespace doris
