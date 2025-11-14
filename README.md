# 外卖管理系统 (Food Delivery Management System)

![C++](https://img.shields.io/badge/Language-C++-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)
![IDE](https://img.shields.io/badge/IDE-Visual%20Studio-purple.svg)

这是一个基于 C++ 开发的控制台版外卖管理系统。该项目模拟了现实生活中外卖平台的核心业务流程，支持**商家**、**顾客**、**骑手**三种角色的交互，并实现了基于 **Dijkstra 算法** 的最短路径配送计算。

## 📖 项目介绍

本项目采用面向对象设计（OOP），模拟了一个完整的外卖生态系统。系统包含用户注册登录、菜单管理、订单处理、路径规划和数据持久化等功能。所有数据（用户、地点、订单、菜单）均通过本地 TXT 文件进行保存和读取，确保数据不会因程序关闭而丢失。

## ✨ 主要功能

### 1. 核心算法
* **最短路径规划**：使用 **Dijkstra 算法** 计算地点之间的最短距离。
    * **顾客端**：根据距离远近对商家进行排序，优先展示距离最近的商家。
    * **骑手端**：计算从当前位置到取餐点、再到送餐点的最短路线，并支持路径节点导航。
* **哈希加密**：使用 BKDRHash 算法对用户密码进行哈希存储，保障基本安全性。

### 2. 角色功能详解

* **🧑‍🎓 顾客 (Customer)**
    * 浏览商家列表（自动按距离排序）。
    * 查看详细菜单、选择菜品并下单。
    * 个人钱包充值与余额管理。
    * 修改当前收货地址。

* **🏪 商家 (Merchant)**
    * 店铺信息管理（修改店铺名称、简介）。
    * 菜单管理（添加新菜品、修改价格/描述、删除菜品）。
    * 订单处理（查看未处理订单、确认接单、标记为待配送）。

* **🛵 骑手 (Rider)**
    * **智能接单**：查看待配送订单池，系统按距离（当前位置 -> 商家）排序推荐。
    * **路径导航**：一键查看配送任务的最短路径（例如：A点 -> B点 -> C点）。
    * 状态更新：接单 -> 配送中 -> 已完成。
    * 位置模拟：配送完成后，骑手位置自动更新至订单终点。

### 3. 数据持久化
系统启动时自动读取以下文件，结束程序时自动保存：
* `places.txt`: 地点名称及地图拓扑结构（路径/距离）。
* `merchants.txt`: 商家账户及店铺信息。
* `customers.txt`: 顾客账户信息。
* `riders.txt`: 骑手账户信息。
* `menus.txt`: 所有商家的菜品数据。
* `orders.txt`: 所有历史及进行中的订单记录。

## 🛠️ 技术栈

* **编程语言**: C++ (Standard Template Library)
* **核心数据结构**: `vector`, `map`, `priority_queue` (用于堆优化 Dijkstra)
* **文件操作**: `fstream` (数据读写)
* **开发环境**: Visual Studio (推荐) / Windows

## 🚀 快速开始

### 环境要求
* **操作系统**: Windows (代码中使用了 `ctime_s` 等 MSVC 特有安全函数)。
* **编译器**: 支持 C++11 及以上标准的编译器 (推荐 MSVC)。

### 编译与运行

1.  **克隆仓库**
    ```bash
    git clone [https://github.com/你的用户名/你的仓库名.git](https://github.com/你的用户名/你的仓库名.git)
    ```

2.  **打开项目**
    * 使用 Visual Studio 打开 `.sln` 解决方案文件，或者直接打开包含 `main.cpp` 的文件夹。

3.  **数据初始化 (重要)**
    * 首次运行时，请确保项目根目录下存在 `places.txt` 文件用于构建地图。
    * 如果文件为空，程序会提示错误。你需要预先录入地点和路径数据（格式参考代码中的 `loadPlaces` 函数）。

4.  **运行**
    * 点击 Visual Studio 的 "Local Windows Debugger" 即可运行。

## 📂 文件结构

```text
.
├── main.cpp           # 项目主程序源代码 (包含所有类定义与逻辑)
├── places.txt         # 地点节点与边权数据
├── merchants.txt      # 商家数据存储
├── customers.txt      # 顾客数据存储
├── riders.txt         # 骑手数据存储
├── menus.txt          # 菜单数据存储
├── orders.txt         # 订单数据存储
└── README.md          # 项目说明文档
