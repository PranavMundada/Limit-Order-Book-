#include <iostream>
#include <map>
#include <list>
#include <cstdint> //for fixed width integers
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
using namespace std;

const int32_t NULL_INDEX = -1;

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

struct OrderNode
{
    uint64_t orderId;
    uint64_t price;
    uint64_t quantity;
    Side side;
    uint64_t timestamp;
    int32_t nextOrderIndex;
};

struct PriceLevel
{
    uint64_t price;
    int32_t headOrderIndex = NULL_INDEX, tailOrderIndex = NULL_INDEX;
};

class OrderBook
{

private:
    std::vector<OrderNode> orderPool;

    int32_t firstFreeIndex = 0;

    std::vector<PriceLevel> asks, bids;

    int32_t bestBidIndex = 0, bestAskIndex = 0;

    int32_t allocateOrder()
    {
        if (firstFreeIndex == NULL_INDEX)
        {
            throw std::runtime_error("The Order Pool is Full... ");
        }
        int32_t allocatedIndex = firstFreeIndex;
        firstFreeIndex = orderPool[allocatedIndex].nextOrderIndex;

        return allocatedIndex;
    }

    void deallocateOrder(int32_t indexToFree)
    {
        orderPool[indexToFree].nextOrderIndex = firstFreeIndex;
        firstFreeIndex = indexToFree;
    }

public:
    OrderBook(size_t maxOrders)
    {
        orderPool.resize(maxOrders);

        for (size_t i = 0; i < maxOrders - 1; i++)
        {
            orderPool[i].nextOrderIndex = i + 1;
        }
        orderPool[maxOrders - 1].nextOrderIndex = NULL_INDEX;
    }

    void processOrder(const Order &incomingOrder)
    {
        int32_t newIndex = allocateOrder();
        orderPool[newIndex] = {incomingOrder.orderId, incomingOrder.price, incomingOrder.quantity, incomingOrder.side, incomingOrder.timestamp};
        orderPool[newIndex].nextOrderIndex = NULL_INDEX;

        if (orderPool[newIndex].side == Side::Buy)
        {
            while (orderPool[newIndex].quantity > 0 && bestAskIndex < asks.size() && asks[bestAskIndex].price <= orderPool[newIndex].price)
            {
                while (bestAskIndex < asks.size() && asks[bestAskIndex].headOrderIndex == NULL_INDEX)
                    bestAskIndex++;
                if (bestAskIndex >= asks.size() || asks[bestAskIndex].price > orderPool[newIndex].price)
                    break;
                int32_t oldestIndex = asks[bestAskIndex].headOrderIndex;
                int32_t tradeQty = min(orderPool[oldestIndex].quantity, orderPool[newIndex].quantity);
                orderPool[oldestIndex].quantity -= tradeQty;
                orderPool[newIndex].quantity -= tradeQty;
                if (orderPool[oldestIndex].quantity == 0)
                {
                    int32_t indexToFree = asks[bestAskIndex].headOrderIndex;
                    asks[bestAskIndex].headOrderIndex = orderPool[oldestIndex].nextOrderIndex;
                    deallocateOrder(indexToFree);
                }
                if (asks[bestAskIndex].headOrderIndex == NULL_INDEX)
                    bestAskIndex++;
            }
        }
        else
        {
            while (orderPool[newIndex].quantity > 0 && bestBidIndex < bids.size() && bids[bestBidIndex].price >= orderPool[newIndex].price)
            {
                while (bestBidIndex < bids.size() && bids[bestBidIndex].headOrderIndex == NULL_INDEX)
                    bestBidIndex++;
                if (bestBidIndex >= bids.size() || bids[bestBidIndex].price < orderPool[newIndex].price)
                    break;
                int32_t oldestIndex = bids[bestBidIndex].headOrderIndex;
                int32_t tradeQty = min(orderPool[oldestIndex].quantity, orderPool[newIndex].quantity);
                orderPool[oldestIndex].quantity -= tradeQty;
                orderPool[newIndex].quantity -= tradeQty;
                if (orderPool[oldestIndex].quantity == 0)
                {
                    int32_t indexToFree = bids[bestBidIndex].headOrderIndex;
                    bids[bestBidIndex].headOrderIndex = orderPool[oldestIndex].nextOrderIndex;
                    deallocateOrder(indexToFree);
                }
                if (bids[bestBidIndex].headOrderIndex == NULL_INDEX)
                    bestBidIndex++;
            }
        }

        if (orderPool[newIndex].quantity > 0)
        {
            if (orderPool[newIndex].side == Side::Buy)
            {
                auto it = std::lower_bound(bids.begin(), bids.end(), incomingOrder.price, [](const PriceLevel &level, uint64_t price)
                                           { return level.price > price; });

                int32_t insertIndex = std::distance(bids.begin(), it);

                if (it != bids.end() && it->price == incomingOrder.price)
                {
                    if (it->headOrderIndex == NULL_INDEX)
                    {
                        it->headOrderIndex = newIndex;
                        it->tailOrderIndex = newIndex;
                    }
                    else
                    {
                        orderPool[it->tailOrderIndex].nextOrderIndex = newIndex;
                        it->tailOrderIndex = newIndex;
                    }
                }
                else
                {
                    PriceLevel newPriceLevel;
                    newPriceLevel.price = incomingOrder.price;
                    newPriceLevel.headOrderIndex = newIndex;
                    newPriceLevel.tailOrderIndex = newIndex;

                    bids.insert(it, newPriceLevel);
                }
                if (insertIndex <= bestBidIndex)
                    bestBidIndex = insertIndex;
            }
            else
            {
                auto it = std::lower_bound(asks.begin(), asks.end(), incomingOrder.price, [](const PriceLevel &level, uint64_t price)
                                           { return level.price < price; });

                int32_t insertIndex = std::distance(asks.begin(), it);

                if (it != asks.end() && it->price == incomingOrder.price)
                {
                    if (it->headOrderIndex == NULL_INDEX)
                    {
                        it->headOrderIndex = newIndex;
                        it->tailOrderIndex = newIndex;
                    }
                    else
                    {
                        orderPool[it->tailOrderIndex].nextOrderIndex = newIndex;
                        it->tailOrderIndex = newIndex;
                    }
                }
                else
                {
                    PriceLevel newPriceLevel;
                    newPriceLevel.price = incomingOrder.price;
                    newPriceLevel.headOrderIndex = newIndex;
                    newPriceLevel.tailOrderIndex = newIndex;

                    asks.insert(it, newPriceLevel);
                }
                if (insertIndex <= bestAskIndex)
                    bestAskIndex = insertIndex;
            }
        }
        else
        {
            deallocateOrder(newIndex);
        }
    }
};

int main()
{
    int numOrders = 1000000;
    OrderBook book(numOrders);
    std::vector<Order> testOrders;

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