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
    // the packs hold their city's day set only (Paris.pak_day, Paris.pvs_Day): a night or overcast race (0x383188,
    // 0x383184) asks for .pak_night / .pak_overcast and the area loader stops the game (0x116D7B) without them; those
    // missing are linked to the day set, so such races run in daylight
    for (const auto& e : fs::directory_iterator(from, ec)) {
        std::string file = e.path().filename().string(), lower = file;
        for (char& c : lower) c = char(std::tolower(static_cast<unsigned char>(c)));
        for (const char* day : {".pak_day", ".pvs_day"}) {
            size_t n = std::strlen(day);
            if (lower.size() <= n || lower.compare(lower.size() - n, n, day) != 0) continue;
            std::string stem = file.substr(0, file.size() - n), kind = std::string(day).substr(0, 5);  // .pak_ or .pvs_
            for (const char* when : {"night", "overcast", "Night", "Overcast"}) {
                std::string other = stem + kind + when;
                if (!kernel::GameFileExists(folder + "\\" + other))
                    kernel::CreateSymbolicLink("\\??\\" + folder + "\\" + other, "\\??\\" + folder + "\\" + file);
            }
        }
    }
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
        bool pack_ini = lower == "cities.ini" || lower == "routes.ini" || lower == "carkeys.ini";  // the pack's own
        if ((ext == ".ini" || ext == ".xwb") && !pack_ini && e.is_regular_file(ec)) {
            std::string audio = "D:\\data\\audio\\" + name;
            if (!kernel::GameFileExists(audio)) kernel::CreateSymbolicLink("\\??\\" + audio, "\\??\\" + folder + "\\" + name);
            continue;
        }
        if (ext != ".pak" || !e.is_regular_file(ec)) continue;
        fs::path dest = to / e.path().filename();
        // the disc's object paks open with an INDX chunk index (then WMSH, MAT, GPUD ...); the packs' boats are in the
        // title update's layout (a zlib compressed WMSH chunk first), which the disc build reads as garbage meshes (huge
        // triangles over the sky, a crash reading their vertices): one of the disc's river boats goes in their place
        char magic[4] = {};
        std::ifstream(e.path(), std::ios::binary).read(magic, 4);
        if (std::memcmp(magic, "INDX", 4) != 0) {
            static const char* kBoats[] = {"StockholmBoat1.pak", "boatBig.pak"};
            static int next = 0;
            const char* boat = kBoats[next++ % 2];
            std::vector<uint8_t> disc;
            if (kernel::ReadGameFile(std::string("D:\\Cache\\Objects\\") + boat, disc)) {
                std::ofstream(dest, std::ios::binary).write(reinterpret_cast<const char*>(disc.data()), std::streamsize(disc.size()));
                LOGR("pgr2: %s from %s is not in the disc's pak layout: the disc's %s stands in", name.c_str(),
                     folder.c_str(), boat);
            }
            continue;
        }
        if (fs::exists(dest, ec) && fs::file_size(dest, ec) == e.file_size(ec)) continue;
        fs::copy_file(e.path(), dest, fs::copy_options::overwrite_existing, ec);
        LOGR("pgr2: %s from %s copied to Z:\\Cars%s", e.path().filename().string().c_str(), folder.c_str(),
             ec ? " FAILED" : "");
    }
}

// The city's draw list sorted into its 106 material buckets (0x11B210, ecx the city renderer: entries of 8 bytes from
// +0x7750, their count at +0x1EF24, an entry's first dword a pointer to its material, whose byte at +2 is the bucket; the
// buckets' write pointers are the table 0x457188, 106 dwords). The packs' title update materials have bucket numbers
// past the table: the sort read a null pointer from beyond it and wrote through it (Harry, Paris turn 1). With
// PGR2_DLC=1 such entries leave the list first (their objects are not drawn, as their error material draws are not).
X86_EXTERN_FUNC(__imp__sub_0011B210);
X86_FUNC(sub_0011B210) {
    if (On()) {
        constexpr uint32_t kBuckets = (0x457330 - 0x457188) / 4;
        uint32_t list = ctx.ecx + 0x7750, count_at = ctx.ecx + 0x1EF24;
        uint32_t n = kernel::G<uint16_t>(count_at), kept = 0;
        for (uint32_t i = 0; i < n; i++) {
            uint32_t e = list + i * 8, material = kernel::R32(e);
            uint32_t bucket = material ? kernel::G<uint8_t>(material + 2) : kBuckets;
            if (bucket >= kBuckets) {
                static int said = 0;
                if (said++ < 8) LOGR("pgr2: a draw list entry with material bucket %u (of %u) left out", bucket, kBuckets);
                continue;
            }
            if (kept != i) {
                kernel::W32(list + kept * 8, kernel::R32(e));
                kernel::W32(list + kept * 8 + 4, kernel::R32(e + 4));
            }
            kept++;
        }
        if (kept != n) kernel::W16(count_at, uint16_t(kept));
    }
    __imp__sub_0011B210(ctx, base);
}
