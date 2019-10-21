![](https://raw.githubusercontent.com/neko-para/qdevcpp/master/qdevcpp.ico)

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

* [x] 自动保存
* [x] 对齐
* [x] 快捷注释
* [x] 行编辑
	* [x] 复制,删除行
	* [x] 移动行
* [x] 状态栏
* [x] 制作windows binary

### v0.1

* [ ] 自更新
* [ ] 命令行参数解析
* [ ] 文件关联
	* [ ] Windows
	* [ ] Linux
		* [ ] Ubuntu

### v0.2

* [ ] 括号折叠
* [ ] 括号补全
* [ ] 拖拽文件
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