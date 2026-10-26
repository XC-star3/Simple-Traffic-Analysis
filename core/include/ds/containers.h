#pragma once

#include <cassert>
#include <cstddef>

namespace ds {

// 自研动态数组：所有核心容器的底层存储。
// 不依赖 STL，满足课程“不使用软件包实现数据结构”的要求。
template <class T>
class Vector {
public:
    Vector() = default;
    explicit Vector(int n) { resize(n); }
    Vector(const Vector& other) { copyFrom(other); }
    Vector& operator=(const Vector& other) {
        if (this != &other) {
            copyFrom(other);
        }
        return *this;
    }
    ~Vector() { delete[] m_data; }

    int size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    int capacity() const { return m_cap; }
    T* data() { return m_data; }
    const T* data() const { return m_data; }

    T& operator[](int i) {
        assert(i >= 0 && i < m_size);
        return m_data[i];
    }
    const T& operator[](int i) const {
        assert(i >= 0 && i < m_size);
        return m_data[i];
    }

    void clear() { m_size = 0; }

    void reserve(int n) {
        if (n <= m_cap) {
            return;
        }
        T* p = new T[n];
        for (int i = 0; i < m_size; ++i) {
            p[i] = m_data[i];
        }
        delete[] m_data;
        m_data = p;
        m_cap = n;
    }

    void resize(int n) {
        if (n > m_cap) {
            reserve(n);
        }
        for (int i = m_size; i < n; ++i) {
            m_data[i] = T();
        }
        m_size = n;
    }

    void push_back(const T& v) {
        if (m_size == m_cap) {
            reserve(m_cap == 0 ? 8 : m_cap * 2);
        }
        m_data[m_size++] = v;
    }

    void pop_back() {
        assert(m_size > 0);
        --m_size;
    }

    T& front() {
        assert(m_size > 0);
        return m_data[0];
    }
    const T& front() const {
        assert(m_size > 0);
        return m_data[0];
    }

    T& back() {
        assert(m_size > 0);
        return m_data[m_size - 1];
    }
    const T& back() const {
        assert(m_size > 0);
        return m_data[m_size - 1];
    }

    void removeAt(int i) {
        assert(i >= 0 && i < m_size);
        for (int k = i; k + 1 < m_size; ++k) {
            m_data[k] = m_data[k + 1];
        }
        --m_size;
    }

    void insertAt(int i, const T& v) {
        assert(i >= 0 && i <= m_size);
        push_back(v);
        for (int k = m_size - 1; k > i; --k) {
            m_data[k] = m_data[k - 1];
        }
        m_data[i] = v;
    }

    void reverse() {
        for (int i = 0, j = m_size - 1; i < j; ++i, --j) {
            T tmp = m_data[i];
            m_data[i] = m_data[j];
            m_data[j] = tmp;
        }
    }

private:
    void copyFrom(const Vector& other) {
        T* p = other.m_size > 0 ? new T[other.m_size] : nullptr;
        for (int i = 0; i < other.m_size; ++i) {
            p[i] = other.m_data[i];
        }
        delete[] m_data;
        m_data = p;
        m_size = other.m_size;
        m_cap = other.m_size;
    }

    T* m_data = nullptr;
    int m_size = 0;
    int m_cap = 0;
};

// 顺序栈：主通道、临时等候区。
template <class T>
class Stack {
public:
    void push(const T& v) { m_items.push_back(v); }
    T pop() {
        T v = m_items.back();
        m_items.pop_back();
        return v;
    }
    T& top() { return m_items.back(); }
    const T& top() const { return m_items.back(); }
    bool empty() const { return m_items.empty(); }
    int size() const { return m_items.size(); }
    void clear() { m_items.clear(); }
    const T& at(int i) const { return m_items[i]; }  // 0 为栈底
private:
    Vector<T> m_items;
};

// 循环队列：三条优先级队列。
template <class T>
class Queue {
public:
    Queue() = default;
    ~Queue() { delete[] m_data; }
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(const T& v) {
        if (m_size == m_cap) {
            grow();
        }
        m_data[(m_head + m_size) % m_cap] = v;
        ++m_size;
    }

    T dequeue() {
        assert(m_size > 0);
        T v = m_data[m_head];
        m_head = (m_head + 1) % m_cap;
        --m_size;
        return v;
    }

    T& front() {
        assert(m_size > 0);
        return m_data[m_head];
    }
    const T& front() const {
        assert(m_size > 0);
        return m_data[m_head];
    }

    bool empty() const { return m_size == 0; }
    int size() const { return m_size; }
    void clear() {
        m_head = 0;
        m_size = 0;
    }

    const T& at(int i) const {
        assert(i >= 0 && i < m_size);
        return m_data[(m_head + i) % m_cap];
    }

    void removeAt(int pos) {
        assert(pos >= 0 && pos < m_size);
        for (int i = pos; i + 1 < m_size; ++i) {
            m_data[(m_head + i) % m_cap] = m_data[(m_head + i + 1) % m_cap];
        }
        --m_size;
    }

private:
    void grow() {
        int nc = m_cap == 0 ? 8 : m_cap * 2;
        T* p = new T[nc];
        for (int i = 0; i < m_size; ++i) {
            p[i] = m_data[(m_head + i) % m_cap];
        }
        delete[] m_data;
        m_data = p;
        m_cap = nc;
        m_head = 0;
    }

    T* m_data = nullptr;
    int m_cap = 0;
    int m_head = 0;
    int m_size = 0;
};

// 二叉小顶堆：Dijkstra 的优先队列。
template <class T>
class MinHeap {
public:
    void push(const T& v) {
        m_items.push_back(v);
        siftUp(m_items.size() - 1);
    }

    T pop() {
        assert(!m_items.empty());
        T top = m_items[0];
        m_items[0] = m_items.back();
        m_items.pop_back();
        if (!m_items.empty()) {
            siftDown(0);
        }
        return top;
    }

    const T& top() const {
        assert(!m_items.empty());
        return m_items[0];
    }
    bool empty() const { return m_items.empty(); }
    int size() const { return m_items.size(); }
    void clear() { m_items.clear(); }

private:
    void swapAt(int a, int b) {
        T tmp = m_items[a];
        m_items[a] = m_items[b];
        m_items[b] = tmp;
    }

    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (!(m_items[i] < m_items[parent])) {
                break;
            }
            swapAt(i, parent);
            i = parent;
        }
    }

    void siftDown(int i) {
        const int n = m_items.size();
        for (;;) {
            int left = i * 2 + 1;
            int right = left + 1;
            int best = i;
            if (left < n && m_items[left] < m_items[best]) {
                best = left;
            }
            if (right < n && m_items[right] < m_items[best]) {
                best = right;
            }
            if (best == i) {
                break;
            }
            swapAt(i, best);
            i = best;
        }
    }

    Vector<T> m_items;
};

// 开放地址法哈希表，键为 int（货车编号 / 站点编号）。
// 值类型 V 需要可默认构造、可拷贝赋值。
template <class V>
class HashMap {
public:
    HashMap() { rehash(16); }

    void put(int key, const V& value) {
        if ((m_used + 1) * 10 >= m_slots.size() * 7) {
            rehash(m_slots.size() * 2);
        }
        insertNoGrow(key, value);
    }

    bool get(int key, V& out) const {
        int p = findSlot(key);
        if (p < 0) {
            return false;
        }
        out = m_slots[p].value;
        return true;
    }

    bool contains(int key) const { return findSlot(key) >= 0; }

    bool remove(int key) {
        int p = findSlot(key);
        if (p < 0) {
            return false;
        }
        m_slots[p].state = 2;  // 墓碑
        --m_size;
        return true;
    }

    void clear() {
        for (int i = 0; i < m_slots.size(); ++i) {
            m_slots[i].state = 0;
            m_slots[i].key = 0;
            m_slots[i].value = V();
        }
        m_size = 0;
        m_used = 0;
    }

    int size() const { return m_size; }
    int capacity() const { return m_slots.size(); }

private:
    struct Slot {
        int key = 0;
        V value{};
        unsigned char state = 0;  // 0 空、1 使用、2 墓碑
    };

    static unsigned hashOf(int key) {
        unsigned x = static_cast<unsigned>(key);
        x ^= x >> 16;
        x *= 0x7feb352dU;
        x ^= x >> 15;
        x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    int findSlot(int key) const {
        const int cap = m_slots.size();
        int i = static_cast<int>(hashOf(key) % static_cast<unsigned>(cap));
        for (int step = 0; step < cap; ++step) {
            const Slot& s = m_slots[i];
            if (s.state == 0) {
                return -1;
            }
            if (s.state == 1 && s.key == key) {
                return i;
            }
            i = (i + 1) % cap;
        }
        return -1;
    }

    void insertNoGrow(int key, const V& value) {
        const int cap = m_slots.size();
        int i = static_cast<int>(hashOf(key) % static_cast<unsigned>(cap));
        int tomb = -1;
        for (;;) {
            Slot& s = m_slots[i];
            if (s.state == 0) {
                int idx = tomb >= 0 ? tomb : i;
                Slot& t = m_slots[idx];
                t.key = key;
                t.value = value;
                t.state = 1;
                if (tomb < 0) {
                    ++m_used;
                }
                ++m_size;
                return;
            }
            if (s.state == 1 && s.key == key) {
                s.value = value;
                return;
            }
            if (s.state == 2 && tomb < 0) {
                tomb = i;
            }
            i = (i + 1) % cap;
        }
    }

    void rehash(int cap) {
        if (cap < 8) {
            cap = 8;
        }
        Vector<Slot> old = m_slots;
        m_slots.clear();
        m_slots.resize(cap);
        for (int i = 0; i < cap; ++i) {
            m_slots[i].state = 0;
            m_slots[i].key = 0;
            m_slots[i].value = V();
        }
        m_size = 0;
        m_used = 0;
        for (int i = 0; i < old.size(); ++i) {
            if (old[i].state == 1) {
                insertNoGrow(old[i].key, old[i].value);
            }
        }
    }

    Vector<Slot> m_slots;
    int m_size = 0;
    int m_used = 0;  // 使用 + 墓碑，用于扩容判断
};

}  // namespace ds
