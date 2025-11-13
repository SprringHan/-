#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <ctime>
#include <algorithm>
#include <memory>
#include <fstream>
#include <queue>
#include <limits>
#include <sstream>
using namespace std;

class Places;
class Meals;
class Order;
class Scene;
class SceneManager;

bool globalShouldExit = false;

unsigned int simpleHash(const string& str) {
    unsigned int seed = 131; // BKDRHash
    unsigned int hash = 0;
    for (char c : str) {
        hash = hash * seed + c;
    }
    return hash;
}
// 地点类
class Places {
public:
    string name;
    vector<pair<Places*, double>> paths; // 到其他地点的距离

    Places(const string& name) : name(name) {}

    void addPath(Places* dest, double distance) {
        paths.push_back({ dest, distance });
    }
};

// 菜品类
class Meals {
public:
    string name;
    double price;
    string description;

    Meals(const string& name = "", double price = 0, const string& desc = "")
        : name(name), price(price), description(desc) {
    }
};

// 订单类
class Order {
public:
    vector<Meals> meals;
    double totalAmount;
    time_t orderTime;
    string status;
    Places* startPlace;
    Places* deliveryPlace;
    string customerName; // 新增：顾客用户名
    string merchantName; // 新增：商家用户名

    Order(const vector<Meals>& meals = {}, double totalAmount = 0, Places* start = nullptr, Places* end = nullptr, const string& customerName = "", const string& merchantName = "")
        : meals(meals), totalAmount(totalAmount), startPlace(start), deliveryPlace(end), customerName(customerName), merchantName(merchantName) {
        orderTime = time(nullptr);
        status = "未完成";
    }
};

// 用户基类
class User {
public:
    string username;
    unsigned int password_hash;
    double balance;
    Places* place;

    User(const string& uname = "", unsigned int pwd_hash = 0, double bal = 0, Places* p = nullptr)
        : username(uname), password_hash(pwd_hash), balance(bal), place(p) {
    }
    virtual ~User() {}
};

// 商家类
class Merchant : public User {
public:
    bool isLoggedIn = false;
    string shopName;      // 店铺名称
    string shopDesc;      // 店铺简介
    vector<Meals> menu;
    vector<Order> orders;

    Merchant(const string& uname = "", unsigned int pwd_hash = 0, double bal = 0, Places* p = nullptr,
        const string& shopName = "", const string& shopDesc = "")
        : User(uname, pwd_hash, bal, p), shopName(shopName), shopDesc(shopDesc) {
    }

    void addMeal(const Meals& meal) {
        menu.push_back(meal);
    }

    void modifyMeal(int idx, const Meals& meal) {
        if (idx >= 0 && idx < menu.size()) {
            menu[idx] = meal;
        }
    }

    void confirmOrder(int orderIdx) {
        if (orderIdx >= 0 && orderIdx < orders.size()) {
            orders[orderIdx].status = "待配送";
        }
    }
};

// 顾客类
class Customer : public User {
public:
    bool isLoggedIn = false;
    vector<Order> orders;

    Customer(const string& uname = "", unsigned int pwd_hash = 0, double bal = 0, Places* p = nullptr)
        : User(uname, pwd_hash, bal, p) {
    }

    void recharge(double amount) {
        balance += amount;
    }

    void placeOrder(Merchant& merchant, const vector<int>& mealIdxs) {
        vector<Meals> selectedMeals;
        double total = 0;
        for (int idx : mealIdxs) {
            if (idx >= 0 && idx < merchant.menu.size()) {
                selectedMeals.push_back(merchant.menu[idx]);
                total += merchant.menu[idx].price;
            }
        }
        if (balance >= total) {
            balance -= total;
            Order order(selectedMeals, total, merchant.place, place, username, merchant.username); // 传入用户名和商家名
            orders.push_back(order);
            merchant.orders.push_back(order);
        }
        else {
            cout << "余额不足，无法下单！" << endl;
        }
    }
};

// 骑手类
class Rider : public User {
public:
    bool isLoggedIn = false;
    vector<Order*> deliveringOrders;

    Rider(const string& uname = "", unsigned int pwd_hash = 0, double bal = 0, Places* p = nullptr)
        : User(uname, pwd_hash, bal, p) {
    }

    void deliverOrder(Order* order) {
        if (order && order->status == "待配送") {
            order->status = "已完成";
            deliveringOrders.push_back(order);
        }
    }
};

// 所有地点管理
class PlacesManager {
public:
    vector<Places> allPlaces;
    map<pair<Places*, Places*>, double> placeMap; // 保存最短距离
    void addPlace(const string& name);
    Places* findPlace(const string& name);

    // 计算所有点对的最短距离
    void updateAllShortestDistances() {
        placeMap.clear();
        for (auto& start : allPlaces) {
            dijkstra(&start);
        }
    }

    // 单源最短路径，记录所有终点的最短距离
    void dijkstra(Places* start) {
        map<Places*, double> dist;
        for (auto& p : allPlaces) {
            dist[&p] = numeric_limits<double>::infinity();
        }
        dist[start] = 0;
        using P = pair<double, Places*>;
        priority_queue<P, vector<P>, greater<P>> pq;
        pq.push({ 0, start });
        while (!pq.empty()) {
            P top = pq.top(); pq.pop();
            double d = top.first;
            Places* u = top.second;
            if (d > dist[u]) continue;
            for (auto& edge : u->paths) {
                Places* v = edge.first;
                double w = edge.second;
                if (dist[v] > dist[u] + w) {
                    dist[v] = dist[u] + w;
                    pq.push({ dist[v], v });
                }
            }
        }
        // 保存所有终点的最短距离
        for (auto& p : allPlaces) {
            if (&p != start && dist[&p] < numeric_limits<double>::infinity()) {
                placeMap[{start, & p}] = dist[&p];
            }
        }
    }
};
vector<Merchant> merchants;
vector<Customer> customers;
vector<Rider> riders;
PlacesManager placesManager;
// 场景基类
class Scene {
public:
    virtual void enter(class SceneManager& manager) = 0;
    virtual ~Scene() {}
};

// 全局场景指针
Scene* globalSignUpScene = nullptr;
Scene* globalCustomerScene = nullptr;
Scene* globalMerchantScene = nullptr;
Scene* globalRiderScene = nullptr;

enum SceneType {
    SIGNUP_SCENE,
    CUSTOMER_SCENE,
    MERCHANT_SCENE,
    RIDER_SCENE
};

class SceneManager {
public:
    Scene* currentScene = nullptr;
    void switchScene(SceneType type, User* user = nullptr) {
        switch (type) {
        case SIGNUP_SCENE:
            for (auto& m : merchants) m.isLoggedIn = false;
            for (auto& c : customers) c.isLoggedIn = false;
            for (auto& r : riders) r.isLoggedIn = false;
            currentScene = globalSignUpScene;
            break;
        case CUSTOMER_SCENE:
            for (auto& c : customers) c.isLoggedIn = false;
            if (user) static_cast<Customer*>(user)->isLoggedIn = true;
            currentScene = globalCustomerScene;
            break;
        case MERCHANT_SCENE:
            for (auto& m : merchants) m.isLoggedIn = false;
            if (user) static_cast<Merchant*>(user)->isLoggedIn = true;
            currentScene = globalMerchantScene;
            break;
        case RIDER_SCENE:
            for (auto& r : riders) r.isLoggedIn = false;
            if (user) static_cast<Rider*>(user)->isLoggedIn = true;
            currentScene = globalRiderScene;
            break;
        }
    }
    void run() {
        while (currentScene && !globalShouldExit) {
            currentScene->enter(*this);
        }
    }
};

// 注册/登录场景
class SignUpScene : public Scene {
public:
    void enter(SceneManager& manager) override {
        cout << "欢迎来到外卖管理系统！" << endl;
        cout << "请选择操作：" << endl;
        cout << "1. 注册" << endl;
        cout << "2. 登录" << endl;
        cout << "0. 退出" << endl;
        int op;
        cin >> op;
        if (op == 1) {
            int type;
            while (true) {
                cout << "请选择注册用户类型：" << endl;
                cout << "1. 商家(Merchant)" << endl;
                cout << "2. 顾客(Customer)" << endl;
                cout << "3. 骑手(Rider)" << endl;
                cout << "请输入数字编号: ";
                if (!(cin >> type)) {
                    cin.clear();
                    cin.ignore(1024, '\n');
                    cout << "输入无效，请输入数字编号。" << endl;
                    continue;
                }
                if (type == 1 || type == 2 || type == 3) break;
                cout << "用户类型选择错误，请重新输入。" << endl;
            }
            string uname, pwd;
            while (true) {
                cout << "请输入用户名: ";
                cin >> uname;
                bool exist = false;
                for (auto& m : merchants) if (m.username == uname) exist = true;
                for (auto& c : customers) if (c.username == uname) exist = true;
                for (auto& r : riders) if (r.username == uname) exist = true;
                if (exist) {
                    cout << "用户名已存在，请重新输入。" << endl;
                }
                else {
                    break;
                }
            }
            cout << "请输入密码: ";
            cin >> pwd;
            unsigned int pwd_hash = simpleHash(pwd);
            double bal = 0; // 初始余额固定为0

            if (placesManager.allPlaces.empty()) {
                cout << "当前无可用地点，请联系管理员维护places.txt。" << endl;
                return;
            }
            int placeIdx = -1;
            while (true) {
                cout << "请选择所在地点编号：" << endl;
                for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
                    cout << i << ". " << placesManager.allPlaces[i].name << endl;
                }
                cin >> placeIdx;
                if (placeIdx >= 0 && placeIdx < placesManager.allPlaces.size()) {
                    break;
                }
                else {
                    cout << "地点选择错误，请重新选择。" << endl;
                }
            }
            Places* p = &placesManager.allPlaces[placeIdx];

            if (type == 1) {
                merchants.emplace_back(uname, pwd_hash, bal, p);
                cout << "商家注册成功！" << endl;
            }
            else if (type == 2) {
                customers.emplace_back(uname, pwd_hash, bal, p);
                cout << "顾客注册成功！" << endl;
            }
            else if (type == 3) {
                riders.emplace_back(uname, pwd_hash, bal, p);
                cout << "骑手注册成功！" << endl;
            }
        }
        else if (op == 2) {
            cout << "请输入用户名: ";
            string uname, pwd;
            cin >> uname;
            cout << "请输入密码: ";
            cin >> pwd;
            unsigned int pwd_hash = simpleHash(pwd);

            for (auto& m : merchants) {
                if (m.username == uname && m.password_hash == pwd_hash) {
                    cout << "商家登录成功！" << endl;
                    manager.switchScene(MERCHANT_SCENE, &m);
                    return;
                }
            }
            for (auto& c : customers) {
                if (c.username == uname && c.password_hash == pwd_hash) {
                    cout << "顾客登录成功！" << endl;
                    manager.switchScene(CUSTOMER_SCENE, &c);
                    return;
                }
            }
            for (auto& r : riders) {
                if (r.username == uname && r.password_hash == pwd_hash) {
                    cout << "骑手登录成功！" << endl;
                    manager.switchScene(RIDER_SCENE, &r);
                    return;
                }
            }
            cout << "用户名或密码错误，登录失败！" << endl;
        }
        else if (op == 0) {
            cout << "感谢使用，程序退出。" << endl;
            globalShouldExit = true;
        }
        else {
            cout << "无效操作，请重新选择。" << endl;
        }
    }
};

// 顾客主场景
class CustomerScene : public Scene {
public:
    CustomerScene() {}
    void enter(SceneManager& manager) override {
        Customer* customer = nullptr;
        for (auto& c : customers) if (c.isLoggedIn) customer = &c;
        if (!customer) { cout << "未检测到已登录顾客，返回登录界面。" << endl; manager.switchScene(SIGNUP_SCENE); return; }
        while (true) {
            cout << "\n===== 顾客主界面 =====" << endl;
            cout << "1. 点餐" << endl;
            cout << "2. 管理个人信息" << endl;
            cout << "0. 退出登录" << endl;
            cout << "请选择操作: ";
            int op;
            cin >> op;
            if (op == 1) {
                // 点餐流程
                // 1. 按距离排序所有商家
                vector<pair<Merchant*, double>> merchantList;
                for (auto& m : merchants) {
                    double dist = numeric_limits<double>::infinity();
                    if (placesManager.placeMap.count({ customer->place, m.place })) {
                        dist = placesManager.placeMap[{customer->place, m.place}];
                    }
                    merchantList.push_back({ &m, dist });
                }
                sort(merchantList.begin(), merchantList.end(), [](const pair<Merchant*, double>& a, const pair<Merchant*, double>& b) {
                    return a.second < b.second;
                    });
                cout << "\n--- 商家列表（按距离排序）---" << endl;
                for (int i = 0; i < merchantList.size(); ++i) {
                    cout << i << ". " << merchantList[i].first->shopName << " | 简介: " << merchantList[i].first->shopDesc << " | 距离: ";
                    if (merchantList[i].second == numeric_limits<double>::infinity()) cout << "不可达";
                    else cout << merchantList[i].second;
                    cout << endl;
                }
                cout << "请选择商家编号（-1返回）: ";
                int midx;
                cin >> midx;
                if (midx == -1) continue;
                if (midx < 0 || midx >= merchantList.size() || merchantList[midx].second == numeric_limits<double>::infinity()) {
                    cout << "编号错误或不可达！" << endl;
                    continue;
                }
                Merchant* m = merchantList[midx].first;
                // 显示菜单
                cout << "\n--- " << m->shopName << " 菜单 ---" << endl;
                for (int i = 0; i < m->menu.size(); ++i) {
                    cout << i << ". " << m->menu[i].name << " ￥" << m->menu[i].price << " - " << m->menu[i].description << endl;
                }
                vector<int> mealIdxs;
                while (true) {
                    cout << "请输入要点的菜品编号（-1结束点单）: ";
                    int idx;
                    cin >> idx;
                    if (idx == -1) break;
                    if (idx < 0 || idx >= m->menu.size()) {
                        cout << "编号错误！" << endl;
                        continue;
                    }
                    mealIdxs.push_back(idx);
                    cout << "已添加: " << m->menu[idx].name << endl;
                }
                if (mealIdxs.empty()) {
                    cout << "未选择任何菜品，返回。" << endl;
                    continue;
                }
                // 计算总价
                double total = 0;
                for (int idx : mealIdxs) total += m->menu[idx].price;
                cout << "订单总价: ￥" << total << endl;
                if (customer->balance < total) {
                    cout << "余额不足，请先充值。" << endl;
                    continue;
                }
                cout << "确认下单？(1确认/0取消): ";
                int confirm;
                cin >> confirm;
                if (confirm != 1) {
                    cout << "已取消。" << endl;
                    continue;
                }
                // 下单
                customer->placeOrder(*m, mealIdxs);
                cout << "下单成功，等待商家处理！" << endl;
            }
            else if (op == 2) {
                while (true) {
                    cout << "\n--- 个人信息管理 ---" << endl;
                    cout << "1. 充值" << endl;
                    cout << "2. 修改地址" << endl;
                    cout << "0. 返回" << endl;
                    cout << "请选择操作: ";
                    int subop;
                    cin >> subop;
                    if (subop == 1) {
                        cout << "请输入充值金额: ";
                        double amount;
                        cin >> amount;
                        if (amount <= 0) {
                            cout << "金额需大于0！" << endl;
                        }
                        else {
                            customer->recharge(amount);
                            cout << "充值成功，当前余额: ￥" << customer->balance << endl;
                        }
                    }
                    else if (subop == 2) {
                        cout << "请选择新地址编号: " << endl;
                        for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
                            cout << i << ". " << placesManager.allPlaces[i].name << endl;
                        }
                        int pidx;
                        cin >> pidx;
                        if (pidx < 0 || pidx >= placesManager.allPlaces.size()) {
                            cout << "编号错误！" << endl;
                        }
                        else {
                            customer->place = &placesManager.allPlaces[pidx];
                            cout << "地址修改成功！" << endl;
                        }
                    }
                    else if (subop == 0) {
                        break;
                    }
                    else {
                        cout << "无效操作，请重新选择。" << endl;
                    }
                }
            }
            else if (op == 0) {
                cout << "已退出登录。" << endl;
                manager.switchScene(SIGNUP_SCENE);
                return;
            }
            else {
                cout << "无效操作，请重新选择。" << endl;
            }
        }
    }
};

// 商家主场景
class MerchantScene : public Scene {
public:
    MerchantScene() {}
    void enter(SceneManager& manager) override {
        Merchant* merchant = nullptr;
        for (auto& m : merchants) if (m.isLoggedIn) merchant = &m;
        if (!merchant) { cout << "未检测到已登录商家，返回登录界面。" << endl; manager.switchScene(SIGNUP_SCENE); return; }
        while (true) {
            cout << "\n===== 商家管理界面 =====" << endl;
            cout << "店铺名称: " << merchant->shopName << endl;
            cout << "店铺简介: " << merchant->shopDesc << endl;
            cout << "1. 查看菜单" << endl;
            cout << "2. 添加菜品" << endl;
            cout << "3. 修改菜品" << endl;
            cout << "4. 删除菜品" << endl;
            cout << "5. 修改店铺信息" << endl;
            cout << "6. 查看和处理订单" << endl;
            cout << "0. 退出登录" << endl;
            cout << "请选择操作: ";
            int op;
            cin >> op;
            if (op == 1) {
                cout << "\n--- 菜单 ---" << endl;
                for (int i = 0; i < merchant->menu.size(); ++i) {
                    cout << i << ". " << merchant->menu[i].name << " ￥" << merchant->menu[i].price << " - " << merchant->menu[i].description << endl;
                }
            }
            else if (op == 2) {
                string name, desc;
                double price;
                cout << "请输入菜品名称: ";
                cin.ignore();
                getline(cin, name);
                cout << "请输入菜品价格: ";
                cin >> price;
                cout << "请输入菜品描述: ";
                cin.ignore();
                getline(cin, desc);
                merchant->addMeal(Meals(name, price, desc));
                cout << "添加成功！" << endl;
            }
            else if (op == 3) {
                cout << "请输入要修改的菜品编号: ";
                int idx;
                cin >> idx;
                if (idx < 0 || idx >= merchant->menu.size()) {
                    cout << "编号错误！" << endl;
                    continue;
                }
                // 显示当前菜品信息
                cout << "当前菜品信息：" << endl;
                cout << "名称: " << merchant->menu[idx].name << endl;
                cout << "价格: ￥" << merchant->menu[idx].price << endl;
                cout << "描述: " << merchant->menu[idx].description << endl;
                string name, desc;
                double price;
                cout << "请输入新菜品名称: ";
                cin.ignore();
                getline(cin, name);
                cout << "请输入新菜品价格: ";
                cin >> price;
                cout << "请输入新菜品描述: ";
                cin.ignore();
                getline(cin, desc);
                merchant->modifyMeal(idx, Meals(name, price, desc));
                cout << "修改成功！" << endl;
            }
            else if (op == 4) {
                cout << "请输入要删除的菜品编号: ";
                int idx;
                cin >> idx;
                if (idx < 0 || idx >= merchant->menu.size()) {
                    cout << "编号错误！" << endl;
                    continue;
                }
                merchant->menu.erase(merchant->menu.begin() + idx);
                cout << "删除成功！" << endl;
            }
            else if (op == 5) {
                cout << "请输入新店铺名称: ";
                cin.ignore();
                getline(cin, merchant->shopName);
                cout << "请输入新店铺简介: ";
                getline(cin, merchant->shopDesc);
                cout << "修改成功！" << endl;
            }
            else if (op == 6) {
                cout << "\n--- 订单列表 ---" << endl;
                for (int i = 0; i < merchant->orders.size(); ++i) {
                    char buf[26];
                    ctime_s(buf, sizeof(buf), &merchant->orders[i].orderTime);
                    cout << i << ". 状态: " << merchant->orders[i].status << ", 金额: ￥" << merchant->orders[i].totalAmount << ", 起点: " << (merchant->orders[i].startPlace ? merchant->orders[i].startPlace->name : "未知") << ", 终点: " << (merchant->orders[i].deliveryPlace ? merchant->orders[i].deliveryPlace->name : "未知") << ", 下单时间: " << buf;
                }
                cout << "请输入要处理的订单编号（-1返回）: ";
                int idx;
                cin >> idx;
                if (idx == -1) continue;
                if (idx < 0 || idx >= merchant->orders.size()) {
                    cout << "编号错误！" << endl;
                    continue;
                }
                Order& order = merchant->orders[idx];
                cout << "订单状态: " << order.status << endl;
                if (order.status == "未完成") {
                    cout << "1. 确认订单(标记为待配送)  0. 返回" << endl;
                    int subop;
                    cin >> subop;
                    if (subop == 1) {
                        merchant->confirmOrder(idx);
                        cout << "订单已确认，状态已更新为待配送。" << endl;
                    }
                }
                else if (order.status == "待配送") {
                    cout << "订单已确认，等待骑手配送。" << endl;
                }
                else if (order.status == "已完成") {
                    cout << "订单已完成。" << endl;
                }
            }
            else if (op == 0) {
                cout << "已退出登录。" << endl;
                manager.switchScene(SIGNUP_SCENE);
                return;
            }
            else {
                cout << "无效操作，请重新选择。" << endl;
            }
        }
    }
};

// 骑手主场景
class RiderScene : public Scene {
public:
    RiderScene() {}
    void enter(SceneManager& manager) override {
        Rider* rider = nullptr;
        for (auto& r : riders) if (r.isLoggedIn) rider = &r;
        if (!rider) { cout << "未检测到已登录骑手，返回登录界面。" << endl; manager.switchScene(SIGNUP_SCENE); return; }
        Order* deliveringOrder = nullptr;
        for (auto* o : rider->deliveringOrders) {
            if (o->status == "配送中") {
                deliveringOrder = o;
                break;
            }
        }
        while (true) {
            cout << "\n===== 骑手主界面 =====" << endl;
            if (deliveringOrder) {
                cout << "当前正在配送订单，起点: "
                    << (deliveringOrder->startPlace ? deliveringOrder->startPlace->name : "未知")
                    << "，终点: "
                    << (deliveringOrder->deliveryPlace ? deliveringOrder->deliveryPlace->name : "未知")
                    << "，状态: " << deliveringOrder->status << endl;
                cout << "1. 完成订单" << endl;
                cout << "2. 查看路径" << endl;
                cout << "3. 修改自身位置" << endl;
                cout << "0. 退出登录" << endl;
                cout << "请选择操作: ";
                int op;
                cin >> op;
                if (op == 1) {
                    cout << "是否已将订单配送到指定地址？(1确认/0取消): ";
                    int confirm;
                    cin >> confirm;
                    if (confirm == 1) {
                        deliveringOrder->status = "已完成";
                        rider->place = deliveringOrder->deliveryPlace;
                        cout << "订单已完成，您的当前位置已更新为终点。" << endl;
                        deliveringOrder = nullptr;
                        rider->deliveringOrders.clear();
                    }
                }
                else if (op == 2) {
                    if (!deliveringOrder->startPlace || !deliveringOrder->deliveryPlace) {
                        cout << "订单起点或终点未知，无法显示路径。" << endl;
                    }
                    else {
                        vector<Places*> path;
                        Places* from = deliveringOrder->startPlace;
                        Places* to = deliveringOrder->deliveryPlace;
                        map<Places*, double> dist;
                        map<Places*, Places*> prev;
                        for (auto& p : placesManager.allPlaces) {
                            dist[&p] = numeric_limits<double>::infinity();
                            prev[&p] = nullptr;
                        }
                        dist[from] = 0;
                        using P = pair<double, Places*>;
                        priority_queue<P, vector<P>, greater<P>> pq;
                        pq.push({ 0, from });
                        while (!pq.empty()) {
                            P top = pq.top(); pq.pop();
                            double d = top.first;
                            Places* u = top.second;
                            if (d > dist[u]) continue;
                            for (auto& edge : u->paths) {
                                Places* v = edge.first;
                                double w = edge.second;
                                if (dist[v] > dist[u] + w) {
                                    dist[v] = dist[u] + w;
                                    prev[v] = u;
                                    pq.push({ dist[v], v });
                                }
                            }
                        }
                        Places* cur = to;
                        while (cur && cur != from) {
                            path.push_back(cur);
                            cur = prev[cur];
                        }
                        if (cur == from) path.push_back(from);
                        reverse(path.begin(), path.end());
                        cout << "最短路径: ";
                        for (int i = 0; i < path.size(); ++i) {
                            cout << path[i]->name;
                            if (i + 1 < path.size()) cout << " -> ";
                        }
                        cout << endl;
                    }
                }
                else if (op == 3) {
                    cout << "请选择新地址编号: " << endl;
                    for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
                        cout << i << ". " << placesManager.allPlaces[i].name << endl;
                    }
                    int pidx;
                    cin >> pidx;
                    if (pidx < 0 || pidx >= placesManager.allPlaces.size()) {
                        cout << "编号错误！" << endl;
                    }
                    else {
                        rider->place = &placesManager.allPlaces[pidx];
                        cout << "地址修改成功！" << endl;
                    }
                }
                else if (op == 0) {
                    cout << "已退出登录。" << endl;
                    manager.switchScene(SIGNUP_SCENE);
                    return;
                }
                else {
                    cout << "无效操作，请重新选择。" << endl;
                }
            }
            else {
                cout << "1. 运送订单" << endl;
                cout << "2. 修改自身位置" << endl;
                cout << "0. 退出登录" << endl;
                cout << "请选择操作: ";
                int op;
                cin >> op;
                if (op == 1) {
                    vector<pair<Order*, double>> orderList;
                    for (auto& m : merchants) {
                        for (auto& o : m.orders) {
                            if (o.status == "待配送") {
                                double dist = numeric_limits<double>::infinity();
                                if (placesManager.placeMap.count({ rider->place, o.startPlace })) {
                                    dist = placesManager.placeMap[{rider->place, o.startPlace}];
                                }
                                orderList.push_back({ &o, dist });
                            }
                        }
                    }
                    sort(orderList.begin(), orderList.end(), [](const pair<Order*, double>& a, const pair<Order*, double>& b) {
                        return a.second < b.second;
                        });
                    if (orderList.empty()) {
                        cout << "当前没有待配送订单。" << endl;
                        continue;
                    }
                    cout << "\n--- 待配送订单列表（按起点距离排序）---" << endl;
                    for (int i = 0; i < orderList.size(); ++i) {
                        Order* o = orderList[i].first;
                        cout << i << ". 起点: " << (o->startPlace ? o->startPlace->name : "未知")
                            << "，终点: " << (o->deliveryPlace ? o->deliveryPlace->name : "未知")
                            << "，金额: ￥" << o->totalAmount
                            << "，距离: ";
                        if (orderList[i].second == numeric_limits<double>::infinity()) cout << "不可达";
                        else cout << orderList[i].second;
                        cout << endl;
                    }
                    cout << "请选择要接单的订单编号（-1返回）: ";
                    int idx;
                    cin >> idx;
                    if (idx == -1) continue;
                    if (idx < 0 || idx >= orderList.size() || orderList[idx].second == numeric_limits<double>::infinity()) {
                        cout << "编号错误或不可达！" << endl;
                        continue;
                    }
                    Order* o = orderList[idx].first;
                    o->status = "配送中";
                    rider->deliveringOrders.clear();
                    rider->deliveringOrders.push_back(o);
                    deliveringOrder = o;
                    cout << "接单成功，订单状态已更新为配送中。" << endl;
                }
                else if (op == 2) {
                    cout << "请选择新地址编号: " << endl;
                    for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
                        cout << i << ". " << placesManager.allPlaces[i].name << endl;
                    }
                    int pidx;
                    cin >> pidx;
                    if (pidx < 0 || pidx >= placesManager.allPlaces.size()) {
                        cout << "编号错误！" << endl;
                    }
                    else {
                        rider->place = &placesManager.allPlaces[pidx];
                        cout << "地址修改成功！" << endl;
                    }
                }
                else if (op == 0) {
                    cout << "已退出登录。" << endl;
                    manager.switchScene(SIGNUP_SCENE);
                    return;
                }
                else {
                    cout << "无效操作，请重新选择。" << endl;
                }
            }
        }
    }
};

// 数据持久化接口
class DataManager {
public:
    void savePlaces() {
        ofstream ofs("places.txt");
        // 保存所有地点名称
        for (const auto& p : placesManager.allPlaces) {
            ofs << p.name << '\n';
        }
        ofs << "#PATHS\n";
        // 保存所有路径
        for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
            const auto& p = placesManager.allPlaces[i];
            for (const auto& path : p.paths) {
                // 找到终点编号
                int destIdx = -1;
                for (int j = 0; j < placesManager.allPlaces.size(); ++j) {
                    if (&placesManager.allPlaces[j] == path.first) { destIdx = j; break; }
                }
                if (destIdx != -1)
                    ofs << i << '|' << destIdx << '|' << path.second << '\n';
            }
        }
    }
    void loadPlaces() {
        placesManager.allPlaces.clear();
        ifstream ifs("places.txt");
        string line;
        vector<string> placeNames;
        vector<tuple<int, int, double>> pathInfos;
        bool inPathSection = false;
        while (getline(ifs, line)) {
            if (line == "#PATHS") {
                inPathSection = true;
                continue;
            }
            if (!inPathSection) {
                if (!line.empty()) placeNames.push_back(line);
            }
            else {
                if (line.empty()) continue;
                stringstream ss(line);
                string fromStr, toStr, distStr;
                getline(ss, fromStr, '|');
                getline(ss, toStr, '|');
                getline(ss, distStr);
                int from = stoi(fromStr);
                int to = stoi(toStr);
                double dist = stod(distStr);
                pathInfos.emplace_back(from, to, dist);
            }
        }
        // 创建所有地点对象
        for (const auto& n : placeNames) {
            placesManager.allPlaces.emplace_back(n);
        }
        // 恢复所有路径
        for (const auto& t : pathInfos) {
            int from = get<0>(t);
            int to = get<1>(t);
            double dist = get<2>(t);
            if (from >= 0 && from < placesManager.allPlaces.size() && to >= 0 && to < placesManager.allPlaces.size()) {
                placesManager.allPlaces[from].addPath(&placesManager.allPlaces[to], dist);
            }
        }
    }
    void saveMerchants() {
        ofstream ofs("merchants.txt");
        for (const auto& m : merchants) {
            int placeIdx = -1;
            for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
                if (&placesManager.allPlaces[i] == m.place) { placeIdx = i; break; }
            }
            ofs << m.username << '|' << m.password_hash << '|' << m.balance << '|' << placeIdx << '|' << m.shopName << '|' << m.shopDesc << '\n';
        }
    }
    void loadMerchants() {
        merchants.clear();
        ifstream ifs("merchants.txt");
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string uname, pwd_hash_str, bal_str, placeIdx_str, shopName, shopDesc;
            getline(ss, uname, '|');
            getline(ss, pwd_hash_str, '|');
            getline(ss, bal_str, '|');
            getline(ss, placeIdx_str, '|');
            getline(ss, shopName, '|');
            getline(ss, shopDesc);
            unsigned int pwd_hash = stoul(pwd_hash_str);
            double bal = stod(bal_str);
            int placeIdx = stoi(placeIdx_str);
            Places* p = (placeIdx >= 0 && placeIdx < placesManager.allPlaces.size()) ? &placesManager.allPlaces[placeIdx] : nullptr;
            merchants.emplace_back(uname, pwd_hash, bal, p, shopName, shopDesc);
        }
    }
    void saveCustomers() {
        ofstream ofs("customers.txt");
        for (const auto& c : customers) {
            int placeIdx = -1;
            for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
                if (&placesManager.allPlaces[i] == c.place) { placeIdx = i; break; }
            }
            ofs << c.username << '|' << c.password_hash << '|' << c.balance << '|' << placeIdx << '\n';
        }
    }
    void loadCustomers() {
        customers.clear();
        ifstream ifs("customers.txt");
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string uname, pwd_hash_str, bal_str, placeIdx_str;
            getline(ss, uname, '|');
            getline(ss, pwd_hash_str, '|');
            getline(ss, bal_str, '|');
            getline(ss, placeIdx_str);
            unsigned int pwd_hash = stoul(pwd_hash_str);
            double bal = stod(bal_str);
            int placeIdx = stoi(placeIdx_str);
            Places* p = (placeIdx >= 0 && placeIdx < placesManager.allPlaces.size()) ? &placesManager.allPlaces[placeIdx] : nullptr;
            customers.emplace_back(uname, pwd_hash, bal, p);
        }
    }
    void saveRiders() {
        ofstream ofs("riders.txt");
        for (const auto& r : riders) {
            int placeIdx = -1;
            for (int i = 0; i < placesManager.allPlaces.size(); ++i) {
                if (&placesManager.allPlaces[i] == r.place) { placeIdx = i; break; }
            }
            ofs << r.username << '|' << r.password_hash << '|' << r.balance << '|' << placeIdx << '\n';
        }
    }
    void loadRiders() {
        riders.clear();
        ifstream ifs("riders.txt");
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string uname, pwd_hash_str, bal_str, placeIdx_str;
            getline(ss, uname, '|');
            getline(ss, pwd_hash_str, '|');
            getline(ss, bal_str, '|');
            getline(ss, placeIdx_str);
            unsigned int pwd_hash = stoul(pwd_hash_str);
            double bal = stod(bal_str);
            int placeIdx = stoi(placeIdx_str);
            Places* p = (placeIdx >= 0 && placeIdx < placesManager.allPlaces.size()) ? &placesManager.allPlaces[placeIdx] : nullptr;
            riders.emplace_back(uname, pwd_hash, bal, p);
        }
    }
    void saveMenus() {
        ofstream ofs("menus.txt");
        for (const auto& m : merchants) {
            ofs << m.username << "|";
            for (int i = 0; i < m.menu.size(); ++i) {
                ofs << m.menu[i].name << "," << m.menu[i].price << "," << m.menu[i].description;
                if (i + 1 < m.menu.size()) ofs << ";";
            }
            ofs << '\n';
        }
    }
    void loadMenus() {
        ifstream ifs("menus.txt");
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            size_t pos = line.find('|');
            if (pos == string::npos) continue;
            string uname = line.substr(0, pos);
            string menuStr = line.substr(pos + 1);
            auto it = find_if(merchants.begin(), merchants.end(), [&](const Merchant& m) { return m.username == uname; });
            if (it == merchants.end()) continue;
            it->menu.clear();
            stringstream ss(menuStr);
            string mealStr;
            while (getline(ss, mealStr, ';')) {
                stringstream ms(mealStr);
                string n, p, d;
                getline(ms, n, ',');
                getline(ms, p, ',');
                getline(ms, d);
                if (!n.empty()) it->menu.emplace_back(n, stod(p), d);
            }
        }
    }
    void saveOrders() {
        ofstream ofs("orders.txt");
        for (const auto& m : merchants) {
            for (const auto& o : m.orders) {
                string customerUname = "";
                for (const auto& c : customers) {
                    // 通过订单的deliveryPlace和顾客的place匹配，找到下单顾客
                    if (c.place == o.deliveryPlace) {
                        // 进一步通过订单时间和金额等辅助判断
                        for (const auto& co : c.orders) {
                            if (co.orderTime == o.orderTime && co.totalAmount == o.totalAmount) {
                                customerUname = c.username;
                                break;
                            }
                        }
                    }
                    if (!customerUname.empty()) break;
                }
                ofs << m.username << '|' << customerUname << '|' << o.totalAmount << '|' << o.orderTime << '|' << o.status << '|';
                ofs << (o.startPlace ? o.startPlace->name : "") << '|' << (o.deliveryPlace ? o.deliveryPlace->name : "") << '|';
                for (int i = 0; i < o.meals.size(); ++i) {
                    ofs << o.meals[i].name << ',' << o.meals[i].price << ',' << o.meals[i].description;
                    if (i + 1 < o.meals.size()) ofs << ';';
                }
                ofs << '\n';
            }
        }
    }
    void loadOrders() {
        for (auto& m : merchants) m.orders.clear();
        for (auto& c : customers) c.orders.clear();
        ifstream ifs("orders.txt");
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string merchantUname, customerUname, totalAmountStr, orderTimeStr, status, startPlaceName, deliveryPlaceName, mealsStr;
            getline(ss, merchantUname, '|');
            getline(ss, customerUname, '|');
            getline(ss, totalAmountStr, '|');
            getline(ss, orderTimeStr, '|');
            getline(ss, status, '|');
            getline(ss, startPlaceName, '|');
            getline(ss, deliveryPlaceName, '|');
            getline(ss, mealsStr);
            double totalAmount = stod(totalAmountStr);
            time_t orderTime = stol(orderTimeStr);
            Places* startPlace = nullptr;
            Places* deliveryPlace = nullptr;
            for (auto& p : placesManager.allPlaces) {
                if (p.name == startPlaceName) startPlace = &p;
                if (p.name == deliveryPlaceName) deliveryPlace = &p;
            }
            vector<Meals> meals;
            stringstream ms(mealsStr);
            string mealStr;
            while (getline(ms, mealStr, ';')) {
                stringstream mealss(mealStr);
                string n, p, d;
                getline(mealss, n, ',');
                getline(mealss, p, ',');
                getline(mealss, d);
                if (!n.empty()) meals.emplace_back(n, stod(p), d);
            }
            // 找到商家
            auto mit = find_if(merchants.begin(), merchants.end(), [&](const Merchant& m) { return m.username == merchantUname; });
            if (mit != merchants.end()) {
                mit->orders.emplace_back(meals, totalAmount, startPlace, deliveryPlace);
                mit->orders.back().orderTime = orderTime;
                mit->orders.back().status = status;
            }
            // 找到顾客
            auto cit = find_if(customers.begin(), customers.end(), [&](const Customer& c) { return c.username == customerUname; });
            if (cit != customers.end()) {
                cit->orders.emplace_back(meals, totalAmount, startPlace, deliveryPlace);
                cit->orders.back().orderTime = orderTime;
                cit->orders.back().status = status;
            }
        }
    }
    // 保存所有数据
    void saveAll() {
        savePlaces();
        saveMerchants();
        saveCustomers();
        saveRiders();
        saveMenus();
        saveOrders();
    }
    // 加载所有数据
    void loadAll() {
        loadPlaces();
        loadMerchants();
        loadCustomers();
        loadRiders();
        loadMenus();
        loadOrders();
    }
};


// 主函数
int main() {
    DataManager dataManager;
    dataManager.loadAll();
    placesManager.updateAllShortestDistances();
    globalSignUpScene = new SignUpScene();
    globalCustomerScene = new CustomerScene();
    globalMerchantScene = new MerchantScene();
    globalRiderScene = new RiderScene();
    SceneManager manager;
    manager.switchScene(SIGNUP_SCENE);
    manager.run();
    dataManager.saveAll();
    // 程序结束时释放全局Scene对象
    delete globalSignUpScene;
    delete globalCustomerScene;
    delete globalMerchantScene;
    delete globalRiderScene;
    return 0;
}
