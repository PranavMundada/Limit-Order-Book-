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
    int32_t nextOrderIndex = NULL_INDEX;
    int32_t prevOrderIndex = NULL_INDEX;
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
    std::vector<PriceLevel> asks, bids;
    std::vector<int32_t> orderMap;

    int32_t firstFreeIndex = 0;
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
        orderMap.resize(maxOrders, NULL_INDEX);

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
        orderPool[newIndex].prevOrderIndex = NULL_INDEX;
        orderMap[incomingOrder.orderId] = newIndex;

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
                    orderMap[orderPool[oldestIndex].orderId] = NULL_INDEX;
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
                    orderMap[orderPool[oldestIndex].orderId] = NULL_INDEX;
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
                        orderPool[newIndex].prevOrderIndex = it->tailOrderIndex;
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
                        orderPool[newIndex].prevOrderIndex = it->tailOrderIndex;
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
            orderMap[incomingOrder.orderId] = NULL_INDEX;
            deallocateOrder(newIndex);
        }
    }

    void cancelOrder(uint64_t orderId)
    {

        if (orderId >= orderMap.size())
            return;
        int32_t targetIndex = orderMap[orderId];
        if (targetIndex == NULL_INDEX)
            return;

        OrderNode &targetNode = orderPool[targetIndex];
        int32_t prevIndex = targetNode.prevOrderIndex;
        int32_t nextIndex = targetNode.nextOrderIndex;

        std::vector<PriceLevel>::iterator it;
        if (targetNode.side == Side::Buy)
        {
            it = std::lower_bound(bids.begin(), bids.end(), targetNode.price, [](const PriceLevel &level, uint64_t price)
                                  { return level.price > price; });
        }
        else
        {
            it = std::lower_bound(asks.begin(), asks.end(), targetNode.price, [](const PriceLevel &level, uint64_t price)
                                  { return level.price < price; });
        }

        if (prevIndex == NULL_INDEX)
        {
            if (nextIndex == NULL_INDEX)
            {
                it->headOrderIndex = NULL_INDEX;
                it->tailOrderIndex = NULL_INDEX;
            }
            else
            {
                it->headOrderIndex = nextIndex;
                orderPool[nextIndex].prevOrderIndex = NULL_INDEX;
            }
        }
        else
        {
            if (nextIndex == NULL_INDEX)
            {
                it->tailOrderIndex = prevIndex;
                orderPool[prevIndex].nextOrderIndex = NULL_INDEX;
            }
            else
            {
                orderPool[prevIndex].nextOrderIndex = nextIndex;
                orderPool[nextIndex].prevOrderIndex = prevIndex;
            }
        }

        orderMap[orderId] = NULL_INDEX;
        deallocateOrder(targetIndex);
    }
};

int main()
{
    int numOrders = 1000000;
    OrderBook book(numOrders);
    std::vector<Order> testOrders;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> bidPriceDist(1000, 1600); // Low prices
    uniform_int_distribution<> askPriceDist(1500, 2000); // High prices
    uniform_int_distribution<> quantityDist(100, 150);

    for (int i = 0; i < numOrders; i++)
    {
        Order newOrder;
        newOrder.quantity = quantityDist(gen);
        newOrder.orderId = i;
        newOrder.timestamp = i;

        // Alternate Buys and Sells so we fill both sides of the book
        if (i % 2 == 0)
        {
            newOrder.side = Side::Buy;
            newOrder.price = bidPriceDist(gen);
        }
        else
        {
            newOrder.side = Side::Sell;
            newOrder.price = askPriceDist(gen);
        }

        testOrders.push_back(newOrder);
    }

    // --- 1. BENCHMARK PROCESS ORDER ---
    auto startInsert = std::chrono::high_resolution_clock::now();
    for (Order &order : testOrders)
    {
        book.processOrder(order);
    }
    auto endInsert = std::chrono::high_resolution_clock::now();
    auto insertMicrosec = std::chrono::duration_cast<std::chrono::microseconds>(endInsert - startInsert).count();

    // --- 2. BENCHMARK CANCEL ORDER (Cancel all odd IDs) ---
    auto startCancel = std::chrono::high_resolution_clock::now();
    for (Order &order : testOrders)
    {
        if (order.orderId % 2 != 0)
        {
            book.cancelOrder(order.orderId);
        }
    }
    auto endCancel = std::chrono::high_resolution_clock::now();
    auto cancelMicrosec = std::chrono::duration_cast<std::chrono::microseconds>(endCancel - startCancel).count();

    // --- 3. PRINT STATS ---
    std::cout << "-----------------------------------\n";
    std::cout << "INSERTION (1,000,000 Orders)\n";
    std::cout << "Total Time:      " << insertMicrosec / 1000.0 << " ms\n";
    std::cout << "Latency:         " << static_cast<double>(insertMicrosec) / numOrders << " us/order\n";
    std::cout << "-----------------------------------\n";
    std::cout << "CANCELLATION (500,000 Orders)\n";
    std::cout << "Total Time:      " << cancelMicrosec / 1000.0 << " ms\n";
    std::cout << "Latency:         " << static_cast<double>(cancelMicrosec) / (numOrders / 2) << " us/order\n";
    std::cout << "-----------------------------------\n";

    return 0;
}