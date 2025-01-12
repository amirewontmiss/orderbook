#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <queue>
#include <iomanip>
#include <chrono>
#include <thread>

struct Order {
    std::string orderId;
    std::string type; // "limit" or "market"
    std::string side; // "buy" or "sell"
    double price;
    int quantity;
    std::chrono::system_clock::time_point timestamp;

    Order(const std::string& id, const std::string& t, const std::string& s, double p, int q)
        : orderId(id), type(t), side(s), price(p), quantity(q), timestamp(std::chrono::system_clock::now()) {}
};

class OrderBook {
private:
    std::map<double, std::queue<Order>, std::greater<double>> buyOrders;
    std::map<double, std::queue<Order>> sellOrders;

    void matchOrders() {
        while (!buyOrders.empty() && !sellOrders.empty()) {
            auto buy = buyOrders.begin();
            auto sell = sellOrders.begin();

            if (buy->first >= sell->first) {
                auto& buyQueue = buy->second;
                auto& sellQueue = sell->second;

                Order& buyOrder = buyQueue.front();
                Order& sellOrder = sellQueue.front();

                int matchedQuantity = std::min(buyOrder.quantity, sellOrder.quantity);
                buyOrder.quantity -= matchedQuantity;
                sellOrder.quantity -= matchedQuantity;

                std::cout << "Matched " << matchedQuantity << " units at price " << sell->first 
                          << " (Buy Order ID: " << buyOrder.orderId << ", Sell Order ID: " << sellOrder.orderId << ")\n";

                if (buyOrder.quantity == 0) {
                    buyQueue.pop();
                    if (buyQueue.empty()) {
                        buyOrders.erase(buy);
                    }
                }

                if (sellOrder.quantity == 0) {
                    sellQueue.pop();
                    if (sellQueue.empty()) {
                        sellOrders.erase(sell);
                    }
                }
            } else {
                break;
            }
        }
    }

    void removeExpiredOrders() {
        auto now = std::chrono::system_clock::now();
        for (auto it = buyOrders.begin(); it != buyOrders.end();) {
            auto& queue = it->second;
            while (!queue.empty() && std::chrono::duration_cast<std::chrono::seconds>(now - queue.front().timestamp).count() > 60) {
                std::cout << "Expired Buy Order: " << queue.front().orderId << "\n";
                queue.pop();
            }
            if (queue.empty()) {
                it = buyOrders.erase(it);
            } else {
                ++it;
            }
        }

        for (auto it = sellOrders.begin(); it != sellOrders.end();) {
            auto& queue = it->second;
            while (!queue.empty() && std::chrono::duration_cast<std::chrono::seconds>(now - queue.front().timestamp).count() > 60) {
                std::cout << "Expired Sell Order: " << queue.front().orderId << "\n";
                queue.pop();
            }
            if (queue.empty()) {
                it = sellOrders.erase(it);
            } else {
                ++it;
            }
        }
    }

public:
    void addOrder(const Order& order) {
        if (order.type == "limit") {
            if (order.side == "buy") {
                buyOrders[order.price].push(order);
            } else if (order.side == "sell") {
                sellOrders[order.price].push(order);
            } else {
                throw std::invalid_argument("Invalid order side");
            }
        } else if (order.type == "market") {
            if (order.side == "buy") {
                while (!sellOrders.empty() && order.quantity > 0) {
                    auto sell = sellOrders.begin();
                    Order& sellOrder = sell->second.front();

                    int matchedQuantity = std::min(order.quantity, sellOrder.quantity);
                    order.quantity -= matchedQuantity;
                    sellOrder.quantity -= matchedQuantity;

                    std::cout << "Matched " << matchedQuantity << " units at price " << sell->first
                              << " (Market Buy Order ID: " << order.orderId << ")\n";

                    if (sellOrder.quantity == 0) {
                        sell->second.pop();
                        if (sell->second.empty()) {
                            sellOrders.erase(sell);
                        }
                    }
                }
            } else if (order.side == "sell") {
                while (!buyOrders.empty() && order.quantity > 0) {
                    auto buy = buyOrders.begin();
                    Order& buyOrder = buy->second.front();

                    int matchedQuantity = std::min(order.quantity, buyOrder.quantity);
                    order.quantity -= matchedQuantity;
                    buyOrder.quantity -= matchedQuantity;

                    std::cout << "Matched " << matchedQuantity << " units at price " << buy->first
                              << " (Market Sell Order ID: " << order.orderId << ")\n";

                    if (buyOrder.quantity == 0) {
                        buy->second.pop();
                        if (buy->second.empty()) {
                            buyOrders.erase(buy);
                        }
                    }
                }
            } else {
                throw std::invalid_argument("Invalid order side");
            }
        } else {
            throw std::invalid_argument("Invalid order type");
        }

        matchOrders();
    }

    void periodicCleanup() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            removeExpiredOrders();
        }
    }

    void printOrderBook() const {
        std::cout << "\nOrder Book:\n";
        std::cout << "Buy Orders:\n";
        for (const auto& [price, queue] : buyOrders) {
            std::cout << "Price: " << price << " | Orders: " << queue.size() << "\n";
        }

        std::cout << "Sell Orders:\n";
        for (const auto& [price, queue] : sellOrders) {
            std::cout << "Price: " << price << " | Orders: " << queue.size() << "\n";
        }
    }
};

class TradingPlatform {
private:
    OrderBook orderBook;

    void showMenu() {
        std::cout << "\nTrading Platform Menu:\n";
        std::cout << "1. Add Limit Order\n";
        std::cout << "2. Add Market Order\n";
        std::cout << "3. View Order Book\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter your choice: ";
    }

    void addLimitOrder() {
        std::string orderId, side;
        double price;
        int quantity;

        std::cout << "Enter Order ID: ";
        std::cin >> orderId;
        std::cout << "Enter Side (buy/sell): ";
        std::cin >> side;
        std::cout << "Enter Price: ";
        std::cin >> price;
        std::cout << "Enter Quantity: ";
        std::cin >> quantity;

        orderBook.addOrder(Order(orderId, "limit", side, price, quantity));
    }

    void addMarketOrder() {
        std::string orderId, side;
        int quantity;

        std::cout << "Enter Order ID: ";
        std::cin >> orderId;
        std::cout << "Enter Side (buy/sell): ";
        std::cin >> side;
        std::cout << "Enter Quantity: ";
        std::cin >> quantity;

        orderBook.addOrder(Order(orderId, "market", side, 0.0, quantity));
    }

public:
    void run() {
        std::thread cleanupThread(&OrderBook::periodicCleanup, &orderBook);
        cleanupThread.detach();

        while (true) {
            showMenu();
            int choice;
            std::cin >> choice;

            switch (choice) {
            case 1:
                addLimitOrder();
                break;
            case 2:
                addMarketOrder();
                break;
            case 3:
                orderBook.printOrderBook();
                break;
            case 4:
                std::cout << "Exiting Trading Platform. Goodbye!\n";

