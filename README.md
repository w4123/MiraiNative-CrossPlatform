# Mirai Native Cross Platform

__强大的跨平台 `mirai` 原生插件加载器__

Mirai Native Cross Platform 可以在Windows, Linux, Android和MacOS上允许native插件与[mirai](https://github.com/mamoe/mirai) 交互。

请注意，插件必须被**重新编译**到对应架构和系统才可以被正常加载。

当前版本支持的具体架构请查看[Releases](https://github.com/w4123/MiraiNative-CrossPlatform/releases)。

## 重新编译提示

在Linux和macOS上，你不需要链接到CQP.dll，因为CQP.dll中的符号会在运行时被动态解析。因此，编译后的文件存在Undefined Symbol是没有问题的(前提是这些符号在CQP.dll中存在)。在MacOS上，你可能编译时需要添加```-Wl,-undefined,dynamic_lookup```参数才能编译成功。

在Android上, 由于Android linker问题, 你需要链接到CQP.dll, 无需设置rpath等信息, 动态库位置会在运行时自动解析(兼容安卓5.0+)

虽然正常来说应该叫so或者dylib文件，但是请重命名为以dll为后缀的文件，并放入plugins文件夹即可。

## 其他提示

此项目不包含MiraiAndroid支持，如果想在MiraiAndroid上使用请参考[MiraiNative-MiraiAndroid](https://github.com/w4123/MiraiNative-MiraiAndroid)


以下为原Mirai-Native Readme

------------

# Mirai Native

__强大的 `mirai` 原生插件加载器__

Mirai Native 支持所有`stdcall`方式导出方法的`DLL`与 [mirai](https://github.com/mamoe/mirai) 交互。

与**大部分**`酷Q`插件兼容，**不支持**`CPK`和解包的`DLL`，需获取`DLL`和`JSON`原文件，`JSON`文件**不支持**注释。

## `Mirai Native` 仅支持 `Windows 32位 JRE`
## [欢迎参与建设`Mirai Native`插件中心](https://github.com/iTXTech/mirai-native/issues/50)

## [Wiki - 开发者和用户必读](https://github.com/iTXTech/mirai-native/wiki)

## [下载 `Mirai Native`](https://github.com/iTXTech/mirai-native/releases)

## 使用 [Mirai Console Loader](https://github.com/iTXTech/mirai-console-loader) 安装`Mirai Native`

* `MCL` 支持自动更新插件，支持设置插件更新频道等功能

`.\mcl --update-package org.itxtech:mirai-native --channel stable --type plugin`

## `Mirai Native Tray`

* 右键`流泪猫猫头`打开 `Mirai Native` 托盘菜单。
* 左键`流泪猫猫头`显示悬浮窗。
* 插画由作者女朋友提供，**版权所有**。

## `mirai Native Plugin Manager`

```
> npm
Mirai Native 插件管理器

/disable <插件Id>   停用指定 Mirai Native 插件
/enable <插件Id>   启用指定 Mirai Native 插件
/info <插件Id>   查看指定 Mirai Native 插件的详细信息
/list    列出所有 Mirai Native 插件
/load <DLL文件名>   加载指定DLL文件
/menu <插件Id> <方法名>   调用指定 Mirai Native 插件的菜单方法
/reload <插件Id>   重新载入指定 Mirai Native 插件
/unload <插件Id>   卸载指定 Mirai Native 插件
```


## 开源许可证

    Copyright (C) 2020-2021 iTX Technologies

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
