/*
 * This file is part of ePipe
 * Copyright (C) 2019, Logical Clocks AB. All rights reserved
 *
 * ePipe is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ePipe is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "SchemabasedMetadataReader.h"

SchemabasedMetadataReader::SchemabasedMetadataReader(MConn connection,
        const bool hopsworks, const int lru_cap)
: NdbDataReader<MetadataLogEntry, MConn>(connection, hopsworks)
, mSchemabasedTable(lru_cap) {

}

void SchemabasedMetadataReader::processAddedandDeleted(MetaQ* data_batch,
    eBulk& bulk) {

  SchemabasedMq* data_queue = mSchemabasedTable.get(mNdbConnection.metadataConnection,
          data_batch);

  createJSON(data_queue, bulk);
}

void SchemabasedMetadataReader::createJSON(SchemabasedMq* data_batch, eBulk&
bulk) {
  for (SchemabasedMq::iterator it = data_batch->begin(); it != data_batch->end(); ++it) {
    SchemabasedMetadataEntry entry = *it;
    LOG_TRACE("create JSON for " << entry.to_string());
    bulk.push(mMetadataLogTable.getLogRemovalHandler(entry
    .getMetadataLogEntry()), entry.mEventCreationTime, entry.to_create_json());
  }
}

SchemabasedMetadataReader::~SchemabasedMetadataReader() {

}

