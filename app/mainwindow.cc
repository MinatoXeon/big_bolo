#include "mainwindow.h"

#include <QCheckBox>
#include <QDebug>
#include <QDragEnterEvent>
#include <QFile>
#include <QLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QTextStream>
#include <QUrl>
#include <iostream>
#include <iterator>
#include <map>

#include "itemdef.h"
#include "itemdelegate.h"
#include "listview.h"
#include "util.h"

MainWindow::MainWindow(std::unique_ptr<bolo::Bolo> &&bolo, QWidget *parent)
    : QMainWindow(parent), mybolo{std::move(bolo)} {
  // set the class member
  standard_item_model = new QStandardItemModel(this);
  list_view = new ListView;
  itemdelegate = new ItemDelegate(this);
  sub_layout = new QHBoxLayout;
  main_layout = new QVBoxLayout;

  // 初始化窗口
  QWidget *widget = new QWidget();
  this->setCentralWidget(widget);

  // 设置列表
  list_view->setItemDelegate(itemdelegate);
  list_view->setSpacing(30);
  list_view->setModel(standard_item_model);
  list_view->setDragEnabled(true);
  list_view->setViewMode(QListView::IconMode);
  list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // 新建备份文件的按钮
  // QPixmap add_image(":/images/Add.jpg");
  new_file.setParent(this);
  new_file.setFont(QFont("Arial", 20, QFont::Black));
  new_file.setText("ADD");
  // new_file.setIcon(add_image);
  // new_file.setIconSize(QSize(100, 100));
  new_file.setMinimumSize(100, 100);

  // 设置标题
  title.setFont(QFont("Times", 25, QFont::Black));
  title.setText("BackUp");
  title.setMinimumSize(100, 100);

  // 设置进度条
  progressbar.setFixedSize(580, 30);
  progressbar.setRange(0, 50000);
  progressbar.setValue(50000);

  // 界面布局
  // sub_layout->addStretch();
  sub_layout->addWidget(&new_file);
  // sub_layout->addStretch();
  // sub_layout->addWidget(&title);
  sub_layout->setSizeConstraint(QLayout::SetFixedSize);

  main_layout->addLayout(sub_layout);
  main_layout->addWidget(list_view);
  main_layout->addWidget(&progressbar);

  centralWidget()->setLayout(main_layout);

  // 初始化已存在的备份文件
  InitData();

  // 开启拖放事件
  this->setAcceptDrops(true);

  // 设置信号槽
  connect(&new_file, &QPushButton::released, this,
          &MainWindow::Show_FileWindow);  // 加号显示本地文件列表
  connect(this, &MainWindow::Get_NewFile, this, &MainWindow::Add_NewFile);  // 实施新增备份文件
  connect(itemdelegate, &ItemDelegate::RequireDetail, this,
          &MainWindow::Show_FileDetail);  // 展示备份文件细节，并给出可使用功能
}

MainWindow::~MainWindow() {}

void MainWindow::InitData() {
  for (auto &it : mybolo->backup_files()) {
    // 获取文件属性
    ItemData my_itemdata;
    my_itemdata.file_name = QString::fromStdString(it.second.filename);
    my_itemdata.id = it.second.id;

    // 添加至管理结构
    QStandardItem *item = new QStandardItem;
    item->setSizeHint(QSize(100, 120));
    item->setData(QVariant::fromValue(my_itemdata), Qt::UserRole);
    item->setToolTip(my_itemdata.file_name);  // 设置鼠标放置显示的文件名
    standard_item_model->appendRow(item);
  }
}

void MainWindow::set_progressbar() {
  // 控制虚拟进度条读条
  progressbar.setValue(0);
  for (int i = 0; i <= 50000; i++) progressbar.setValue(i);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  //获取鼠标所拖动的信息
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
  else
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event) {
  // 获取MIME数据
  const QMimeData *mime_data = event->mimeData();

  if (mime_data->hasUrls()) {
    // 获取URL列表
    QList<QUrl> url_list = mime_data->urls();

    // 将其中第一个URL表示为本地文件路径
    QString file_name = url_list.at(0).toLocalFile();
    if (!file_name.isEmpty()) {
      emit Get_NewFile(file_name);
    }
  }
}

void MainWindow::Show_FileWindow() {
  // 设置文件显示窗口属性
  file_window.setWindowTitle("本地文件");
  file_window.setAcceptMode(QFileDialog::AcceptOpen);
  file_window.setViewMode(QFileDialog::List);
  file_window.setFileMode(QFileDialog::Directory);

  // 获取选择文件夹
  if (file_window.exec() == QFileDialog::Accepted) {
    QStringList file_names = file_window.selectedFiles();
    emit Get_NewFile(file_names[0]);
  }
}

void MainWindow::Add_NewFile(QString file_path) {
  // 可选项选择
  QMessageBox set_box(QMessageBox::NoIcon, "OPTIONS", "", 0, NULL, Qt::Sheet);
  set_box.setStyleSheet("background-color:white");

  QCheckBox option_compress("Compress", &set_box);
  QCheckBox option_encrypt("Encrypt", &set_box);
  QCheckBox option_cloud("Cloud-Backup", &set_box);

  set_box.addButton(QMessageBox::No);
  set_box.button(QMessageBox::No)->setHidden(true);

  QPushButton *option_ok = set_box.addButton("Confirm", QMessageBox::AcceptRole);
  QPushButton *option_cancel = set_box.addButton("Cancel", QMessageBox::AcceptRole);
  // QLabel *pLabel = new QLabel("备份可选项");

  // dynamic_cast<QGridLayout *>(set_box.layout())->addWidget(pLabel, 0, 1, 1, 4);
  dynamic_cast<QGridLayout *>(set_box.layout())->addWidget(&option_compress, 0, 1, 1, 4);
  dynamic_cast<QGridLayout *>(set_box.layout())->addWidget(&option_encrypt, 1, 1, 1, 4);
  dynamic_cast<QGridLayout *>(set_box.layout())->addWidget(&option_cloud, 2, 1, 1, 4);
  dynamic_cast<QGridLayout *>(set_box.layout())->addWidget(option_ok, 4, 1);
  dynamic_cast<QGridLayout *>(set_box.layout())->addWidget(option_cancel, 4, 2);

  set_box.exec();

  bool is_compress = option_compress.checkState();
  bool is_encrypt = option_encrypt.checkState();
  bool is_cloud = option_cloud.checkState();

  // 取消新增
  if (set_box.clickedButton() != option_ok) {
    set_box.close();
    return;
  }

  // 有加密需求
  if (is_encrypt) {
    password_window.exec();
    if (password_window.clickedButton() == password_window.option_cancel) {
      password_window.password.setText("");
      return;
    }
    if (password_window.clickedButton() == password_window.option_ok &&
        password_window.password.text().size() == 0) {
      QMessageBox::critical(NULL, "Error", "Password cannot be empty", QMessageBox::Yes, QMessageBox::Yes);
      return;
    }
  }

  // 虚拟进度条读条
  set_progressbar();

  auto res = mybolo->Backup(file_path.toStdString(), is_compress, is_encrypt, is_cloud,
                            password_window.password.text().toStdString());

  if (!res) {
    QMessageBox::critical(NULL, "Error", QString::fromStdString(res.error()), QMessageBox::Yes,
                          QMessageBox::Yes);
    return;
  }

  // 添加新文件
  ItemData my_itemdata;
  my_itemdata.id = res.value().id;
  my_itemdata.file_name = QString::fromStdString(res.value().filename);

  QStandardItem *item = new QStandardItem;
  item->setSizeHint(QSize(100, 120));
  item->setData(QVariant::fromValue(my_itemdata), Qt::UserRole);
  item->setToolTip(my_itemdata.file_name);
  standard_item_model->appendRow(item);

  // 清空密码框内容
  password_window.password.setText("");
}

void MainWindow::Show_FileDetail(const QModelIndex &index) {
  // 获取item存储的数据
  QVariant variant = index.data(Qt::UserRole);
  ItemData data = variant.value<ItemData>();
  auto file = mybolo->GetBackupFile(data.id);
  if (!file) {
    QMessageBox::critical(NULL, "Error", "Cannot read file", QMessageBox::Yes, QMessageBox::Yes);
    return;
  }
  auto open_backupfile = file.value();

  // set the detail text
  QString detail = "";
  detail = detail + "BackUp File: \t" + QString::fromStdString(open_backupfile.filename) + "\n";
  detail = detail + "Original Dir: \t" + QString::fromStdString(open_backupfile.path) + "\n";
  detail = detail + "BackUp Dir: \t" + QString::fromStdString(open_backupfile.backup_path) + "\n";
  detail = detail + "BackUp Time: \t" +
           QString::fromStdString(bolo::TimestampToString(open_backupfile.timestamp)) + "\n";
  detail = detail + "Compressed? \t" + (open_backupfile.is_compressed ? "Yes" : "No") + "\n";
  detail = detail + "Encrypted? \t\t" + (open_backupfile.is_encrypted ? "Yes" : "No") + "\n";
  detail = detail + "InCloud? \t\t" + (open_backupfile.is_in_cloud ? "Yes" : "No") + "\n";

  // set the detail box
  QMessageBox file_detail(QMessageBox::NoIcon, "File Detail", detail, 0, NULL);

  // set the button
  QPushButton *delete_button = file_detail.addButton(tr("Delete"), QMessageBox::AcceptRole);
  QPushButton *update_button = file_detail.addButton(tr("Update"), QMessageBox::AcceptRole);
  QPushButton *restore_button = file_detail.addButton(tr("Restore"), QMessageBox::AcceptRole);
  QPushButton *close_button = file_detail.addButton(tr("Close"), QMessageBox::AcceptRole);

  // 使右上叉号有效
  file_detail.addButton(QMessageBox::No);
  file_detail.button(QMessageBox::No)->setHidden(true);

  file_detail.exec();
  if (file_detail.clickedButton() == restore_button) {
    // 恢复备份操作
    if (open_backupfile.is_encrypted) {
      password_window.exec();
      if (password_window.clickedButton() == password_window.option_cancel) {
        password_window.password.setText("");
        return;
      }
    }
    // 设置并显示文件列表
    file_window.setWindowTitle("Local File");
    file_window.setAcceptMode(QFileDialog::AcceptOpen);
    file_window.setViewMode(QFileDialog::List);
    file_window.setFileMode(QFileDialog::Directory);

    if (file_window.exec() != QFileDialog::Accepted)
      file_window.close();
    else {
      // 虚拟进度条读条
      set_progressbar();

      QStringList file_names = file_window.selectedFiles();
      auto res = mybolo->Restore(open_backupfile.id, file_names[0].toStdString(),
                                 password_window.password.text().toStdString());
      password_window.password.setText("");

      // 输出错误信息
      if (res)
        QMessageBox::critical(NULL, "Error", QString::fromStdString(res.error()), QMessageBox::Yes,
                              QMessageBox::Yes);
    }
  } else if (file_detail.clickedButton() == update_button) {
    // 更新备份
    if (open_backupfile.is_encrypted) {
      password_window.exec();
      if (password_window.clickedButton() == password_window.option_cancel) {
        password_window.password.setText("");
        return;
      }
    }
    // 虚拟进度条读条
    set_progressbar();
    auto res = mybolo->Update(open_backupfile.id, password_window.password.text().toStdString());
    password_window.password.setText("");

    // 输出错误信息
    if (res)
      QMessageBox::critical(NULL, "Error", QString::fromStdString(res.error()), QMessageBox::Yes,
                            QMessageBox::Yes);
  } else if (file_detail.clickedButton() == delete_button) {
    // 删除备份
    // 进行确认选项，允许用户错误点击
    QMessageBox delete_sure(QMessageBox::Warning, "Warning", "Are you sure to delete this backup?",
                            QMessageBox::Yes | QMessageBox::No, NULL);

    int res = delete_sure.exec();
    if (res == QMessageBox::Yes) {
      // 执行删除操作
      // 虚拟进度条读条
      set_progressbar();
      auto res = mybolo->Remove(open_backupfile.id);
      if (res)
        // 输出错误信息
        QMessageBox::critical(NULL, "Error", QString::fromStdString(res.error()), QMessageBox::Yes,
                              QMessageBox::Yes);
      else
        // 结构中删除对应数据
        list_view->model()->removeRow(index.row());
    }
  } else if (file_detail.clickedButton() == close_button)
    // 关闭窗口
    file_detail.close();
}
