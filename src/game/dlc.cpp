// Xbox Live content packs on the disc's default.xbe (PGR2_DLC=1, an experiment). The Paris and Long Beach booster
// packs (T:\$C\4d53004b0000000N) are category 0x10, made for the title update; the disc game lists category 1 (car)
// and 2 (city) packs only. On a console the packs are signed when installed; the copies here are not, so with the
// switch on the content signature checks are passed over.
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <system_error>
#include <cctype>

#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {

// D:\data\audio\stereo.ini lists each city's ambience, "City Day|Night|Rain volume "file.wav"", and Audio::Manager::Setup
// stops the game (0x12C1DF) when the city has no line: a pack's own stereo.ini (its city's lines) is added to the
// disc's in T:\$PGR2\stereo.ini, which the disc's name then links to
void MergeStereo(const std::filesystem::path& pack_stereo) {
    static std::string merged;
    static bool read = false;
    if (!read) {
        std::vector<uint8_t> disc;
        kernel::ReadGameFile("D:\\data\\audio\\stereo.ini", disc);
        merged.assign(disc.begin(), disc.end());
        read = true;
    }
    std::ifstream in(pack_stereo, std::ios::binary);
    std::string lines((std::istreambuf_iterator<char>(in)), {});
    if (!merged.empty() && merged.back() != '\n') merged += "\r\n";
    merged += lines;
    std::error_code ec;
    std::filesystem::path dir = kernel::ResolvePath("T:\\$PGR2", 0);
    if (dir.empty()) return;
    std::filesystem::create_directories(dir, ec);
    std::ofstream(dir / "stereo.ini", std::ios::binary) << merged;
    kernel::CreateSymbolicLink("\\??\\D:\\data\\audio\\stereo.ini", "\\??\\T:\\$PGR2\\stereo.ini");
    LOGR("pgr2: %s added to the city ambience list", pack_stereo.string().c_str());
}

bool On() {
    static const bool on = [] {
        const char* e = std::getenv("PGR2_DLC");
        bool v = e && *e == '1';
        if (v) LOGR("pgr2: PGR2_DLC=1, booster packs listed as city packs, content signatures not checked");
        return v;
    }();
    return on;
}

}  // namespace

// XAPI's XFindFirstContent (0x192134: path, category mask, find data): the city pack listing (mask 2, 0x1574F0) also
// takes the booster packs' category 0x10
X86_EXTERN_FUNC(__imp__sub_00192134);
X86_FUNC(sub_00192134) {
    if (On() && kernel::R32(ctx.esp + 8) == 2) kernel::W32(ctx.esp + 8, 2 | 0x10);
    __imp__sub_00192134(ctx, base);
}

// XAPI: the signature of a content header (0x191366: file, title, header, out; stdcall), compared by 0x19144B with the
// header's first 20 bytes: the header's own, so an unsigned pack passes
X86_EXTERN_FUNC(__imp__sub_00191366);
X86_FUNC(sub_00191366) {
    if (!On()) {
        __imp__sub_00191366(ctx, base);
        return;
    }
    uint32_t header = kernel::R32(ctx.esp + 12), out = kernel::R32(ctx.esp + 16);
    std::memcpy(kernel::HostPtr(out), kernel::HostPtr(header), 20);
    ctx.eax = 1;
    ctx.esp += 20;  // ret 16
}

// PGR2: each file of a pack against its signature in ContentMeta.xbx (0x156F00: metadata, folder; cdecl): passed
X86_EXTERN_FUNC(__imp__sub_00156F00);
X86_FUNC(sub_00156F00) {
    if (!On()) {
        __imp__sub_00156F00(ctx, base);
        return;
    }
    ctx.eax = 1;
    ctx.esp += 4;  // ret
}

// PGR2: a city pack loaded (0x1572F0: metadata, pack folder T:\$C\..., ...; 1 when loaded), its files put where the city
// loads them from. A pack's plain .pak files are
// object paks (Paris's river boats, boat01.pak and boat02.pak) that the city loads from the cache, Z:\Cars, as it does
// the disc's (copied there from D:\Cache\Objects at boot); nothing on the disc build copies a pack's, so they are
// copied here, as an install would
X86_EXTERN_FUNC(__imp__sub_001572F0);
X86_FUNC(sub_001572F0) {
    uint32_t folder_va = kernel::R32(ctx.esp + 8);
    __imp__sub_001572F0(ctx, base);
    if (!On() || !ctx.eax || !folder_va) return;
    std::string folder;
    for (uint32_t p = folder_va; kernel::G<uint8_t>(p) && folder.size() < 260; p++) folder += char(kernel::G<uint8_t>(p));
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path from = kernel::ResolvePath(folder, 0);
    fs::path to = kernel::ResolvePath("Z:\\Cars", 0);
    if (from.empty() || to.empty()) return;
    for (const auto& e : fs::directory_iterator(from, ec)) {
        std::string ext = e.path().extension().string();
        for (char& c : ext) c = char(std::tolower(static_cast<unsigned char>(c)));
        std::string name = e.path().filename().string();
        std::string lower = name;
        for (char& c : lower) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (lower == "stereo.ini") {
            MergeStereo(e.path());
            continue;
        }
        // the city's sound (its Refs, Mono, Night and Rain settings, its dry and wet banks) is read from
        // D:\data\audio as the disc cities' is: linked there, where the disc has no file of that name
        if ((ext == ".ini" || ext == ".xwb") && e.is_regular_file(ec)) {
            std::string audio = "D:\\data\\audio\\" + name;
            if (!kernel::GameFileExists(audio)) kernel::CreateSymbolicLink("\\??\\" + audio, "\\??\\" + folder + "\\" + name);
            continue;
        }
        if (ext != ".pak" || !e.is_regular_file(ec)) continue;
        fs::path dest = to / e.path().filename();
        if (fs::exists(dest, ec) && fs::file_size(dest, ec) == e.file_size(ec)) continue;
        fs::copy_file(e.path(), dest, fs::copy_options::overwrite_existing, ec);
        LOGR("pgr2: %s from %s copied to Z:\\Cars%s", e.path().filename().string().c_str(), folder.c_str(),
             ec ? " FAILED" : "");
    }
}
