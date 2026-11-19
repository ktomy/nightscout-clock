#ifndef GLUCOSEREADINGBUFFER_H
#define GLUCOSEREADINGBUFFER_H

#include <algorithm>
#include <cstddef>
#include <iterator>

// Glucose readings in epoch order, oldest first, in a fixed array inside the object, so storing a
// reading never allocates.
//
// It replaces a std::list, which took one heap block of about 32 bytes per reading. With one-minute
// readings, the data source's list and the display's copy held about 360 of them, 11 KB that was
// missing from every TLS handshake, and the handshake runs out of memory first.
//
// Reading needs an `epoch` member. Arduino-free, so the host tests use it.
template <typename Reading, size_t Capacity>
class GlucoseReadingBuffer {
public:
    GlucoseReadingBuffer() = default;
    // A copy is several kilobytes, too much for a task's stack: pass a reference. Assignment stays,
    // for the display's own copy.
    GlucoseReadingBuffer(const GlucoseReadingBuffer&) = delete;
    GlucoseReadingBuffer& operator=(const GlucoseReadingBuffer& other) {
        if (this != &other) {
            std::copy(other.items, other.items + other.count, items);
            count = other.count;
        }
        return *this;
    }

    static constexpr size_t capacity() { return Capacity; }
    size_t size() const { return count; }
    bool empty() const { return count == 0; }
    void clear() { count = 0; }

    const Reading* begin() const { return items; }
    const Reading* end() const { return items + count; }
    std::reverse_iterator<const Reading*> rbegin() const {
        return std::reverse_iterator<const Reading*>(end());
    }
    std::reverse_iterator<const Reading*> rend() const {
        return std::reverse_iterator<const Reading*>(begin());
    }
    // Only when not empty.
    const Reading& front() const { return items[0]; }
    const Reading& back() const { return items[count - 1]; }

    // Puts the reading in epoch order and returns whether it was added. A reading with the epoch of
    // one already stored is left out, because sources return readings the clock already has. When
    // the buffer is full, the oldest reading makes room, and a reading older than all of them is
    // left out.
    bool add(const Reading& reading) {
        // New readings are usually the newest, so search from the end.
        size_t position = count;
        while (position > 0 && items[position - 1].epoch > reading.epoch) {
            position--;
        }
        if (position > 0 && items[position - 1].epoch == reading.epoch) {
            return false;
        }
        if (count == Capacity) {
            if (position == 0) {
                return false;
            }
            std::move(items + 1, items + position, items);
            items[position - 1] = reading;
            return true;
        }
        std::move_backward(items + position, items + count, items + count + 1);
        items[position] = reading;
        count++;
        return true;
    }

    // Adds each reading of a container, in any order, and returns how many were added.
    template <typename Readings>
    size_t addAll(const Readings& readings) {
        size_t added = 0;
        for (const auto& reading : readings) {
            if (add(reading)) {
                added++;
            }
        }
        return added;
    }

    // Removes the readings older than `epoch` and returns how many.
    size_t removeOlderThan(unsigned long long epoch) {
        size_t removed = 0;
        while (removed < count && items[removed].epoch < epoch) {
            removed++;
        }
        std::move(items + removed, items + count, items);
        count -= removed;
        return removed;
    }

private:
    Reading items[Capacity];
    size_t count = 0;
};

#endif  // GLUCOSEREADINGBUFFER_H
