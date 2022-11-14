#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QStyledItemDelegate>

class ItemDelegate : public QStyledItemDelegate {
  Q_OBJECT

 public:
  explicit ItemDelegate(QObject *parent = 0);
  ~ItemDelegate();

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                   const QModelIndex &index);

 signals:
  void RequireDetail(const QModelIndex &);
};

#endif  // ITEMDELEGATE_H
