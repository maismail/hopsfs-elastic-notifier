/*
 * This file is part of ePipe
 * Copyright (C) 2019, Logical Clocks AB. All rights reserved
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef METADATALOGTABLE_H
#define METADATALOGTABLE_H
#include "DBWatchTable.h"
#include "ConcurrentPriorityQueue.h"
#include "HopsworksOpsLogTable.h"

#define XATTR_FIELD_NAME "xattr"

struct MetadataKey {
  Int32 mId;
  Int32 mFieldId;
  Int32 mTupleId;

  MetadataKey() {
  }

  MetadataKey(Int32 id, Int32 fieldId, Int32 tupleId) {
    mId = id;
    mFieldId = fieldId;
    mTupleId = tupleId;
  }

  bool operator==(const MetadataKey &other) const {
    return (mId == other.mId) && (mFieldId == other.mFieldId) && (mTupleId == other
    .mTupleId);
  }

  std::string getPKStr(){
    std::stringstream stream;
    stream << mId << "-" << mTupleId << "-" << mFieldId;
    return stream.str();
  }
};

struct MetadataKeyHasher {

  std::size_t operator()(const MetadataKey &key) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, boost::hash_value(key.mId));
    boost::hash_combine(seed, boost::hash_value(key.mFieldId));
    boost::hash_combine(seed, boost::hash_value(key.mTupleId));
    return seed;
  }
};

struct MetadataLogEntry {
  Int32 mId;
  MetadataKey mMetaPK;
  HopsworksOpType mMetaOpType;
  ptime mEventCreationTime;

  std::string to_string() {
    std::stringstream stream;
    stream << "-------------------------" << std::endl;
    stream << "Id = " << mId << std::endl;
    stream << "MetaPK = " << mMetaPK.getPKStr() << std::endl;
    stream << "MetaOpType = " << HopsworksOpTypeToStr(mMetaOpType) << std::endl;
    stream << "-------------------------" << std::endl;
    return stream.str();
  }
};

struct MetadataLogEntryComparator {

  bool operator()(const MetadataLogEntry &r1, const MetadataLogEntry &r2) const {
    return r1.mId > r2.mId;
  }
};

typedef ConcurrentPriorityQueue<MetadataLogEntry, MetadataLogEntryComparator> CMetaQ;
typedef std::vector<MetadataLogEntry> MetaQ;

class MetadataLogTable : public DBWatchTable<MetadataLogEntry> {
public:

  MetadataLogTable() : DBWatchTable("meta_log") {
    addColumn("id");
    addColumn("meta_id");
    addColumn("meta_field_id");
    addColumn("meta_tuple_id");
    addColumn("meta_op_type");
    addRecoveryIndex(PRIMARY_INDEX);
    addWatchEvent(NdbDictionary::Event::TE_INSERT);
  }

  MetadataLogEntry getRow(NdbRecAttr* value[]) {
    MetadataLogEntry row;
    row.mEventCreationTime = Utils::getCurrentTime();
    row.mId = value[0]->int32_value();
    Int32 metaId = value[1]->int32_value();
    Int32 metaFieldId = value[2]->int32_value();
    Int32 metaTupleId = value[3]->int32_value();
    row.mMetaPK = MetadataKey(metaId, metaFieldId, metaTupleId);
    row.mMetaOpType = static_cast<HopsworksOpType> (value[4]->int8_value());
    return row;
  }

  void removeLogs(Ndb* conn, UISet& pks) {
    start(conn);
    for (UISet::iterator it = pks.begin(); it != pks.end(); ++it) {
      int id = *it;
      doDelete(id);
      LOG_TRACE("Delete log row: " << id);
    }
    end();
  }

  std::string getPKStr(MetadataLogEntry row) override {
    return row.mMetaPK.getPKStr();
  }
};


#endif /* METADATALOGTABLE_H */

