/*
 * Copyright (C) 2018 Hops.io
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
/*
 * File:   XAttrTable.h
 * Author: Mahmoud Ismail <maism@kth.se>
 *
 */

#ifndef EPIPE_XATTRTABLE_H
#define EPIPE_XATTRTABLE_H
#include "DBTable.h"
#include "FsMutationsLogTable.h"

#define XATTR_FIELD_NAME "xattr"

struct XAttrPK {
  Int64 mInodeId;
  Int8 mNamespace;
  string mName;

  XAttrPK(Int64 inodeId, Int8 nameSpace, string name) {
    mInodeId = inodeId;
    mNamespace = nameSpace;
    mName = name;
  }

  string to_string() {
    stringstream out;
    out << mInodeId << "-" << mNamespace << "-" << mName;
    return out.str();
  }

  AnyMap readKey() {
    AnyMap key;
    key[0] = mInodeId;
    key[1] = mNamespace;
    key[2] = mName;
  }
};

struct XAttrRow {
  Int64 mInodeId;
  Int8 mNamespace;
  string mName;
  string mValue;

  string to_string(){
    stringstream stream;
    stream << "-------------------------" << endl;
    stream << "InodeId = " << mInodeId << endl;
    stream << "Namespace = " << (int)mNamespace << endl;
    stream << "Name = " << mName << endl;
    stream << "Value = " << mValue << endl;
    stream << "-------------------------" << endl;
    return stream.str();
  }
};

typedef vector<XAttrRow> XAttrVec;
typedef boost::unordered_map<string, XAttrVec> XAttrMap;

class XAttrTable : public DBTable<XAttrRow> {

public:

  XAttrTable(string table_name) : DBTable(table_name){
    addColumn("inode_id");
    addColumn("namespace");
    addColumn("name");
    addColumn("value");
  }

  XAttrRow getRow(NdbRecAttr* values[]) {
    XAttrRow row;
    row.mInodeId = values[0]->int64_value();
    row.mNamespace = values[1]->int8_value();
    row.mName = get_string(values[2]);
    row.mValue = get_string(values[3]);
    return row;
  }

  XAttrRow get(Ndb* connection, Int64 inodeId, Int8 ns, string name) {
    AnyMap a;
    a[0] = inodeId;
    a[1] = ns;
    a[2] = name;
    return DBTable<XAttrRow>::doRead(connection, a);
  }

  XAttrMap get(Ndb* connection, Fmq* data_batch) {
    AnyVec anyVec;
    Fmq batchedMutations;

    Fmq addAllXattrs;

    for (Fmq::iterator it = data_batch->begin();
         it != data_batch->end(); ++it) {
      FsMutationRow row = *it;
      if (!row.requiresReadingXAttr() || !row.isXAttrOperation()) {
        continue;
      }

      if(row.mOperation == XAttrAddAll){
        addAllXattrs.push_back(row);
        continue;
      }

      AnyMap pk;
      pk[0] = row.mInodeId;
      pk[1] = row.getNamespace();
      pk[2] = row.getXAttrName();
      anyVec.push_back(pk);
      batchedMutations.push_back(row);
    }

    XAttrVec xattrs = doRead(connection, anyVec);

    XAttrMap results;

    int i=0;
    for(XAttrVec::iterator it = xattrs.begin(); it != xattrs.end(); ++it, i++){
      XAttrRow xattr = *it;
      FsMutationRow mr = batchedMutations[i];

      XAttrVec xvec;
      xvec.push_back(xattr);
      results[mr.getPKStr()] = xvec;
    }

    for(Fmq::iterator it = addAllXattrs.begin(); it != addAllXattrs.end();
    ++it){
      FsMutationRow mr = *it;
      results[mr.getPKStr()] = getByInodeId(connection, mr.mInodeId);
    }

    return results;
  }

  XAttrVec getByInodeId(Ndb* connection, Int64 inodeId){
    AnyMap args;
    args[0] = inodeId;
    return doRead(connection, PRIMARY_INDEX, args);
  }

};
#endif //EPIPE_XATTRTABLE_H
