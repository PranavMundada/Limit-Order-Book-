#include <iostream>
#include <map>
#include <list>
#include <cstdint> //for fixed width integers
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
using namespace std;

enum class Side
{
    Buy,
    Sell
};

struct Order
{
    uint64_t orderId;
    uint64_t price;
    uint64_t quantity;
    Side side;
    uint64_t timestamp;
};

class OrderBook
{

private:
    // buyers sorted from highest price to lowest price
    std::map<uint64_t, std::list<Order>, std::greater<uint64_t>> bids;

    // sellers sorted from lowest price to highest price
    std::map<uint64_t, std::list<Order>> asks;

public:
    OrderBook() = default;

    void addOrder(const Order &order)
    {
        if (order.side == Side::Buy)
        {
            bids[order.price].push_back(order);
        }
        else if (order.side == Side::Sell)
        {
            asks[order.price].push_back(order);
        }
    }

    void processOrder(Order &incomingOrder)
    {
        if (incomingOrder.side == Side::Buy)
        {
            while (incomingOrder.quantity > 0 && !asks.empty())
            {
                if (incomingOrder.price < asks.begin()->first)
                    break;
                Order &restingOrder = asks.begin()->second.front();
                uint64_t tradeQuantity = std::min(incomingOrder.quantity, restingOrder.quantity);
                incomingOrder.quantity -= tradeQuantity;
                restingOrder.quantity -= tradeQuantity;
                if (restingOrder.quantity == 0)
                {
                    asks.begin()->second.pop_front();
                }
                if (asks.begin()->second.empty())
                    asks.erase(asks.begin());
            }
            if (incomingOrder.quantity > 0)
            {
                bids[incomingOrder.price].push_back(incomingOrder);
            }
        }
        else if (incomingOrder.side == Side::Sell)
        {
            while (incomingOrder.quantity > 0 && !bids.empty())
            {
                if (incomingOrder.price > bids.begin()->first)
                    break;
                Order &restingOrder = bids.begin()->second.front();
                uint64_t tradeQuantity = std::min(incomingOrder.quantity, restingOrder.quantity);
                incomingOrder.quantity -= tradeQuantity;
                restingOrder.quantity -= tradeQuantity;
                if (restingOrder.quantity == 0)
                {
                    bids.begin()->second.pop_front();
                }
                if (bids.begin()->second.empty())
                    bids.erase(bids.begin());
            }
            if (incomingOrder.quantity > 0)
            {
                asks[incomingOrder.price].push_back(incomingOrder);
            }
        }
    }

    void printBook()
    {
    }
};

int main()
{
    OrderBook book;
    std::vector<Order> testOrders;

    int numOrders = 1000000;
    testOrders.reserve(numOrders);

    random_device rd;  // seed
    mt19937 gen(rd()); // Mersenne Twister RNG
    uniform_int_distribution<> priceDist(1000, 1500);
    uniform_int_distribution<> quantityDist(100, 150);

    for (int i = 0; i < numOrders; i++)
    {
        Order newOrder;
        uint64_t randomPrice = priceDist(gen);

        if ((randomPrice >> 1) & 1)
        {
            newOrder.side = Side::Buy;
        }
        else
        {
            newOrder.side = Side::Sell;
        }

        newOrder.price = randomPrice;
        newOrder.quantity = quantityDist(gen);
        newOrder.orderId = i;
        newOrder.timestamp = i;
        testOrders.push_back(newOrder);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (Order &order : testOrders)
    {
        book.processOrder(order);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto totalMicrosec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    double avgLatency = static_cast<double>(totalMicrosec) / numOrders;

    std::cout << "-----------------------------------\n";
    std::cout << "Processed:       " << numOrders << " orders\n";
    std::cout << "Total Time:      " << totalMicrosec / 1000.0 << " milliseconds\n";
    std::cout << "Average Latency: " << avgLatency << " microseconds per order\n";
    std::cout << "Throughput:      " << (numOrders / (totalMicrosec / 1000000.0)) << " orders per second\n";
    std::cout << "-----------------------------------\n";

    return 0;
}