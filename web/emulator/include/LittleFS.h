// Emulator stand-in: an in-memory LittleFS, so SettingsManager's real load/save code runs unchanged.
#ifndef EMU_LITTLEFS_H
#define EMU_LITTLEFS_H

#include <Arduino.h>

#include <map>
#include <memory>
#include <string>

#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

std::map<std::string, std::string>& emuFiles();

class File : public Stream {
public:
    File() = default;
    File(const std::string& path, bool writing) : path(path), writing(writing), valid(true) {}

    explicit operator bool() const { return valid; }
    bool isDirectory() const { return false; }
    size_t size() const { return valid ? emuFiles()[path].size() : 0; }
    const char* name() const { return path.c_str(); }
    void close() { valid = false; }
    size_t position() const { return pos; }
    bool seek(size_t to) {
        if (!valid || to > size())
            return false;
        pos = to;
        return true;
    }

    int available() override { return valid && !writing ? (int)(emuFiles()[path].size() - pos) : 0; }
    int read() override {
        if (available() <= 0)
            return -1;
        return (uint8_t)emuFiles()[path][pos++];
    }
    int peek() override {
        if (available() <= 0)
            return -1;
        return (uint8_t)emuFiles()[path][pos];
    }
    size_t write(uint8_t c) override {
        if (!valid || !writing)
            return 0;
        emuFiles()[path].push_back((char)c);
        return 1;
    }
    size_t write(const uint8_t* buffer, size_t size) override {
        if (!valid || !writing)
            return 0;
        emuFiles()[path].append((const char*)buffer, size);
        return size;
    }

private:
    std::string path;
    bool writing = false;
    bool valid = false;
    size_t pos = 0;
};

class LittleFSFS {
public:
    bool begin(bool formatOnFail = false) { return true; }
    void end() {}
    bool exists(const char* path) { return emuFiles().count(path) > 0; }
    bool exists(const String& path) { return exists(path.c_str()); }
    File open(const char* path, const char* mode = FILE_READ, bool create = false) {
        if (mode[0] == 'w') {
            emuFiles()[path] = "";
            return File(path, true);
        }
        if (mode[0] == 'a') {
            emuFiles()[path];
            return File(path, true);
        }
        return exists(path) ? File(path, false) : File();
    }
    File open(const String& path, const char* mode = FILE_READ) { return open(path.c_str(), mode); }
    bool remove(const char* path) { return emuFiles().erase(path) > 0; }
    bool rename(const char* from, const char* to) {
        auto it = emuFiles().find(from);
        if (it == emuFiles().end())
            return false;
        std::string content = it->second;
        emuFiles().erase(it);
        emuFiles()[to] = content;
        return true;
    }
};

extern LittleFSFS LittleFS;

#endif
