#pragma once

#include <atomic>
#include <utility>

namespace util
{
    template <typename T>
    class LockfreeStack
    {
    public:
        struct Node
        {
            T value;
            Node* next;
        };

        static void delete_nodes(Node* head)
        {
            for (auto node = head, next_node = head; node; node = next_node) {
                next_node = node->next;
                delete node;
            }
        }

        template <typename U>
        void push(U&& value)
        {
            auto new_node = new Node{
                .value = std::forward<T>(value),
                .next = _head.load(std::memory_order_relaxed)
            };

            while (!_head.compare_exchange_weak(new_node->next, new_node,
                std::memory_order_release, std::memory_order_relaxed));
        }

        auto pop()
        {
            return std::unique_ptr<Node, decltype(delete_nodes)*>(
                _head.exchange(nullptr, std::memory_order_relaxed),
                delete_nodes
            );
        }

        bool empty() const
        {
            return _head.load(std::memory_order_relaxed) == nullptr;
        }

    private:
        std::atomic<Node*> _head;
    };
}