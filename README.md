![qdevpp icon](https://raw.githubusercontent.com/neko-para/qdevcpp/master/qdevcpp/qdevcpp.ico)

# QDevCpp

以OIer为用户群体的DevC++的升~~降~~级版。

删除了所有项目、版本管理功能，删除了大部分配置选项。

支持Windows & Linux。

[DevC++项目地址](https://sourceforge.net/projects/orwelldevcpp/)

This is not a generic IDE. If you want to learn the c/c++ programing language, don't use it.

## Release

### Windows

#### Mingw

[静态编译 64位](https://github.com/neko-para/qdevcpp/releases/download/v0.0/qdevcpp-mingw-x64-static-release.7z)

[静态编译 32位](https://github.com/neko-para/qdevcpp/releases/download/v0.0/qdevcpp-mingw-x86-static-release.7z)

## Build

使用Qt5和QScintilla2

### QScintilla2

#### Windows

##### Visual Studio

```shell
vcpkg install qscintilla
```

##### MSYS2

```shell
pacman -S mingw-w64-x86_64-qscintilla
```

#### Linux

##### Ubuntu/Debian

```shell
sudo apt install libqscintilla2-qt5-dev
```

## TODOs

### v0.0

### v0.1

* [ ] 文件关联
  * [x] Windows
  * [x] Linux
  * [x] 关联文件图标
* [x] 滚轮缩放
* [x] 拖拽文件
* [ ] 制作安装包
  * [ ] 合并mingw-w64

### v0.2

* [ ] 括号折叠
* [ ] 括号补全
* [ ] 同一文件多窗口编辑
  * [ ] 左右,上下分栏

### v0.3

* [ ] 调试功能

### v0.4

* [ ] 自定义配色
* [ ] 中文GCC错误信息支持?
* [ ] 交互题
* [ ] 适用于比赛的项目
  * [ ] 执行文件输入输出重定向
