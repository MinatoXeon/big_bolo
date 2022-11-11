#ifndef ITEMDEF_H
#define ITEMDEF_H

#include <QMetaType>

#include "bolo.h"

struct ItemData {
  bolo::BackupFileId id;
  QString file_name;
};

Q_DECLARE_METATYPE(ItemData)

#endif  // ITEMDEF_H
