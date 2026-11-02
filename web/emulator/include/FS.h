// Emulator stand-in: melody_factory.h names fs::FS as a default argument; files go through LittleFS.h.
#ifndef EMU_FS_H
#define EMU_FS_H

namespace fs {
class FS {};
}  // namespace fs
using fs::FS;

#endif
