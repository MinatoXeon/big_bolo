# Backup

Backup is is a file archiver used for backup.

//环境配置命令

//提供编译程序必要的软件包的列表信息
sudo apt-get install -y build-essential

//安装openssl相关服务
apt-get install libssl-dev

//安装clang
sudo apt install clang

//安装cmake
sudo snap install cmake --classic

//提供编译程序必要的软件包的列表信息
sudo apt-get install build-essential

//安装qt5
sudo apt-get install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
sudo apt-get install qtcreator
sudo apt-get install qt5*

//下载安装包并安装rclone
sudo dpkg -i '/home/minatoxeon/桌面/rclone-v1.60.0-linux-amd64.deb' 
也可使用命令行命令“curl https://rclone.org/install.sh | sudo bash”来进行安装rclone

//运行rclone相关服务时报错安装的依赖包
sudo apt-get install fuse

//rclone配置教程
https://www.bilibili.com/read/cv16130034/

//软件运行命令

//创建build文件夹
mkdir build

//进入build文件夹
cd ./build

//编译该项目
cmake ..
make

//进入app文件夹
cd ./app

//修改脚本路径文件中的挂载文件夹的路径之后再运行项目
./app “脚本路径文件的路径”
