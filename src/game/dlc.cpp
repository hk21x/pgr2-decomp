// Xbox Live content packs on the disc's default.xbe (the dlc setting or PGR2_DLC=1, an experiment). The Paris and Long Beach booster
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
#include <map>
#include <set>

#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace pgr2 {
extern bool dlc;  // the setting (src/common/game_info.cpp), or PGR2_DLC=1
}

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

// A pak in the title update's layout (each chunk inline: tag, 0, size with 0x80000000 compressed and 0x40000000, its data)
// into the disc's: an INDX chunk listing tag, 0, size and offset (from byte 12) for each, the data 16 byte aligned
// after it, END last. The chunks themselves are the same (the same tags, order and fixed sizes; compressed ones stay as
// they are: a 4 byte size, then the game's deflate stream). Empty when it is not one.
std::vector<uint8_t> ConvertPak(const std::vector<uint8_t>& in) {
    struct Chunk {
        uint32_t tag, size;
        size_t at;
    };
    std::vector<Chunk> chunks;
    size_t o = 0;
    auto u32 = [&](size_t at) {
        uint32_t v;
        std::memcpy(&v, &in[at], 4);
        return v;
    };
    while (o + 12 <= in.size()) {
        Chunk c{u32(o), u32(o + 8), o + 12};
        size_t n = c.size & 0x3FFFFFFF;
        if (c.at + n > in.size()) return {};
        chunks.push_back(c);
        o = c.at + n;
        if (std::memcmp(&c.tag, "END", 3) == 0) break;
    }
    if (chunks.empty() || std::memcmp(&chunks.back().tag, "END", 3) != 0) return {};
    std::vector<uint8_t> out;
    auto put = [&](uint32_t v) { out.insert(out.end(), reinterpret_cast<uint8_t*>(&v), reinterpret_cast<uint8_t*>(&v) + 4); };
    auto align = [](size_t v) { return (v + 15) & ~size_t(15); };
    // the header and the table, then each chunk's data at the next 16 bytes; offsets count from byte 12
    size_t at = align(16 + chunks.size() * 16);
    std::vector<size_t> offsets;
    for (const Chunk& c : chunks) {
        offsets.push_back(at);
        at = align(at + (c.size & 0x3FFFFFFF));
    }
    uint32_t indx;
    std::memcpy(&indx, "INDX", 4);
    put(indx);
    put(0);
    put(uint32_t(offsets.back() - 12));  // to END, as the disc's give it
    put(uint32_t(chunks.size()));
    for (size_t i = 0; i < chunks.size(); i++) {
        put(chunks[i].tag);
        put(0);
        put(chunks[i].size);
        put(uint32_t(offsets[i] - 12));
    }
    out.resize(at, 0);
    for (size_t i = 0; i < chunks.size(); i++)
        std::memcpy(&out[offsets[i]], &in[chunks[i].at], chunks[i].size & 0x3FFFFFFF);
    // the file ends with END's own header (tag, 0, 0) where its entry points
    put(chunks.back().tag);
    put(0);
    put(0);
    return out;
}

// The packs' lists are UTF-16 with a byte order mark, every character ASCII: to and from narrow text
std::string Narrow(const std::vector<uint8_t>& in) {
    std::string out;
    for (size_t i = (in.size() >= 2 && in[0] == 0xFF && in[1] == 0xFE) ? 2 : 0; i + 1 < in.size(); i += 2) out += char(in[i]);
    return out;
}

std::vector<uint8_t> Wide(const std::string& in) {
    std::vector<uint8_t> out = {0xFF, 0xFE};
    for (char c : in) out.push_back(uint8_t(c)), out.push_back(0);
    return out;
}

std::vector<std::string> Lines(const std::string& text) {
    std::vector<std::string> lines;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(at, end - at);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
        at = end + 1;
    }
    return lines;
}

// The value of a "Key = value" line, quotes taken off, or empty when the line is another key's
std::string Value(const std::string& line, const char* key) {
    size_t n = std::strlen(key);
    if (line.compare(0, n, key) != 0) return {};
    size_t eq = line.find('=', n);
    if (eq == std::string::npos || line.find_first_not_of(' ', n) != eq) return {};
    std::string v = line.substr(eq + 1);
    v.erase(0, v.find_first_not_of(" \t\""));
    while (!v.empty() && (v.back() == ' ' || v.back() == '\t' || v.back() == '"')) v.pop_back();
    return v;
}

// The disc's colour list (0x32F1F0) holds 600 (0x14A8C0; the section handler 0x14DD80 writes through the null it gives
// past that): the disc has 566, the update 48 of its own. The update's cars keep as many of their colours as fit, taken
// in turn so each car has some; cars.ini loses the colour lines left out and CarColours.ini their sections. Free: the slots left.
void TrimColours(std::string& cars, std::string& colours, int free) {
    std::set<std::string> own;  // the update's colours; the others a car names are the disc's, which take no slot
    for (const std::string& l : Lines(colours))
        if (std::string v = Value(l, "Name"); !v.empty()) own.insert(v);
    std::vector<std::string> lines = Lines(cars);
    std::vector<std::vector<std::string>> car_colours;
    for (const std::string& l : lines) {
        if (l == "[CAR]") car_colours.emplace_back();
        std::string v = Value(l, "Colour");
        if (!v.empty() && !car_colours.empty()) car_colours.back().push_back(v);
    }
    std::set<std::string> keep;
    std::vector<size_t> taken(car_colours.size(), 0);
    for (bool more = true; more;) {
        more = false;
        for (size_t c = 0; c < car_colours.size(); c++) {
            if (taken[c] == car_colours[c].size()) continue;
            const std::string& name = car_colours[c][taken[c]];
            if (own.count(name) && !keep.count(name)) {
                if (int(keep.size()) >= free) continue;
                keep.insert(name);
            }
            taken[c]++;
            more = true;
        }
    }
    std::string out;
    size_t car = 0, n = 0;
    bool first = true;
    for (const std::string& l : lines) {
        if (l == "[CAR]") car = first ? 0 : car + 1, n = 0, first = false;
        std::string v = Value(l, "Colour");
        if (!v.empty() && !first && n++ >= taken[car]) continue;
        out += l + "\r\n";
    }
    cars = out;
    // CarColours.ini: a section runs from its [CAR COLOUR] to the next
    out.clear();
    std::string section;
    bool kept = false;
    auto flush = [&] {
        if (kept || section.rfind("[CAR COLOUR]", 0) != 0) out += section;
        section.clear();
    };
    for (const std::string& l : Lines(colours)) {
        if (l == "[CAR COLOUR]") flush(), kept = false;
        std::string v = Value(l, "Name");
        if (!v.empty()) kept = keep.count(v) != 0;
        section += l + "\r\n";
    }
    flush();
    colours = out;
}

bool On() {
    static const bool on = [] {
        if (pgr2::dlc) LOGR("pgr2: DLC cities on, booster packs listed as city packs, content signatures not checked");
        return pgr2::dlc;
    }();
    return on;
}

}  // namespace

// The title update's cars as a car pack. The update (T:\$U on a console, any folder in T:\$C here: the one holding its
// cars.ini and dcontentcar paks) brings 16 cars: a cars.ini listing just them in the disc's format (FileName
// DContentCarN, AudioData, colours, class "Paris Cars" ...), CarColours, Manufacturers and Classes, each car's model
// (dcontentcarN.pak_hrd / _opn / _sft, in the update's pak layout), handling (dcontentcarN.ini, as the disc's
// CarDynamics\<car>.ini) and engine sound (<AudioData>.dat / .xwb, _AI, _PCM). The disc build loads car packs, category 1
// (0x157180: those four lists from the pack, each new car's record keeping the pack's folder): they are copied into one,
// T:\$C\4d53004b0000FF01, the models converted, every car free in the showroom; its ContentMeta.xbx is a city pack's with
// the offer and category changed (the signature checks are passed over). A stamp file keeps it from being made twice.
constexpr uint32_t kBuiltCategory = 0x20;  // the built car pack's content category (BuildCarPack)

void BuildCarPack() {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path packs = kernel::ResolvePath("T:\\$C", 0);
    if (packs.empty() || !fs::is_directory(packs, ec)) return;
    auto lower = [](std::string v) {
        for (char& c : v) c = char(std::tolower(static_cast<unsigned char>(c)));
        return v;
    };
    fs::path source, meta;
    for (const auto& d : fs::directory_iterator(packs, ec)) {
        if (!d.is_directory(ec)) continue;
        bool cars = false, models = false;
        for (const auto& f : fs::directory_iterator(d.path(), ec)) {
            std::string n = lower(f.path().filename().string());
            cars |= n == "cars.ini";
            models |= n.rfind("dcontentcar", 0) == 0 && n.find(".pak_") != std::string::npos;
            if (n == "contentmeta.xbx" && lower(d.path().filename().string()).rfind("4d53004b0000000", 0) == 0) meta = f.path();
        }
        if (cars && models && lower(d.path().filename().string()) != "4d53004b0000ff01") source = d.path();
    }
    if (source.empty() || meta.empty()) {
        if (!source.empty()) LOGR("pgr2: DLC cars found in %s, but no city pack to take a ContentMeta.xbx from", source.string().c_str());
        return;
    }
    fs::path dest = packs / "4d53004b0000FF01";
    std::string stamp = "pgr2 car pack 4 from " + source.filename().string();
    {
        std::ifstream in(dest / "pgr2_built.txt");
        std::string have((std::istreambuf_iterator<char>(in)), {});
        if (have == stamp) return;
    }
    fs::create_directories(dest, ec);
    std::string content;
    int converted = 0, cars = 0;
    std::map<std::string, std::string> lists, names;
    for (const auto& f : fs::directory_iterator(source, ec)) {
        if (!f.is_regular_file(ec)) continue;
        std::string name = f.path().filename().string(), n = lower(name);
        // the update's own files, and lists the car loader does not read
        if (n == "default.xbe" || n == "downloader.xbe" || n == "autoupdatemeta.xbx" || n == "contentmeta.xbx" ||
            n == "content.txt" || n == "packid.txt" || n == "races.ini" || n.find("credits") != std::string::npos)
            continue;
        std::ifstream in(f.path(), std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
        if (n.find(".pak") != std::string::npos && !(bytes.size() >= 4 && std::memcmp(bytes.data(), "INDX", 4) == 0)) {
            std::vector<uint8_t> out = ConvertPak(bytes);
            if (out.empty()) {
                LOGW("pgr2: %s could not be converted, left out", name.c_str());
                continue;
            }
            bytes = std::move(out);
            converted++;
        }
        if (n == "cars.ini" || n == "carcolours.ini") {
            lists[n] = Narrow(bytes);
            names[n] = name;
            continue;  // written below, once both are read
        }
        std::ofstream(dest / name, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        content += name + "\r\n";
    }
    if (lists.count("cars.ini")) {
        std::string& list = lists["cars.ini"];
        // every car owned from the start
        for (size_t at; (at = list.find("ShowroomFree = \"0\"")) != std::string::npos; cars++) list[at + 16] = '1';
        std::vector<uint8_t> disc;
        kernel::ReadGameFile("D:\\Frontend\\CarColours.ini", disc);
        size_t used = 0;
        for (const std::string& l : Lines(Narrow(disc))) used += l == "[CAR COLOUR]";
        int free = 600 - int(used);
        if (lists.count("carcolours.ini")) TrimColours(list, lists["carcolours.ini"], free);
        LOGR("pgr2: %zu car colours on the disc, the DLC cars keep %d", used, free);
    }
    for (const auto& [n, text] : lists) {
        std::vector<uint8_t> bytes = Wide(text);
        std::ofstream(dest / names[n], std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        content += names[n] + "\r\n";
    }
    std::ifstream min(meta, std::ios::binary);
    std::vector<uint8_t> m((std::istreambuf_iterator<char>(min)), {});
    if (m.size() >= 0x34) {
        // ContentMeta.xbx: offer id's low dword +0x28, category +0x30. Not 1, the car packs' own: the disc game lists
        // those whatever the setting, and with DLC off would check this one's signature and call it damaged. With DLC
        // on the car pack listing (mask 1) takes kBuiltCategory too.
        uint32_t offer = 0xFF01, category = kBuiltCategory;
        std::memcpy(&m[0x28], &offer, 4);
        std::memcpy(&m[0x30], &category, 4);
    }
    // its name in every language section (the city pack's it was taken from), same length
    const std::pair<std::u16string, std::u16string> renames[] = {{u"Long Beach Booster Pack", u"DLC Cars (title update)"},
                                                               {u"Paris Booster Pack", u"DLC Cars (TU pack)"}};
    for (const auto& [from, to] : renames) {
        size_t len = from.size() * 2;
        for (size_t i = 0; i + len <= m.size(); i++)
            if (std::memcmp(&m[i], from.data(), len) == 0) std::memcpy(&m[i], to.data(), len);
    }
    std::ofstream(dest / "ContentMeta.xbx", std::ios::binary).write(reinterpret_cast<const char*>(m.data()), std::streamsize(m.size()));
    std::ofstream(dest / "Content.txt", std::ios::binary) << content;
    std::ofstream(dest / "PackID.txt", std::ios::binary) << "100";
    std::ofstream(dest / "pgr2_built.txt", std::ios::binary) << stamp;
    LOGR("pgr2: the title update's cars from %s made into the car pack %s (%d models converted, %d cars free)",
         source.string().c_str(), dest.string().c_str(), converted, cars);
}

// XAPI's XFindFirstContent (0x192134: path, category mask, find data): the city pack listing (mask 2, 0x1574F0) also
// takes the booster packs' category 0x10, and the car pack listing (mask 1) the built car pack's
X86_EXTERN_FUNC(__imp__sub_00192134);
X86_FUNC(sub_00192134) {
    if (On()) {
        static bool built = (BuildCarPack(), true);
        (void)built;
    }
    if (On() && kernel::R32(ctx.esp + 8) == 2) kernel::W32(ctx.esp + 8, 2 | 0x10);
    if (On() && kernel::R32(ctx.esp + 8) == 1) kernel::W32(ctx.esp + 8, 1 | kBuiltCategory);  // the built car pack
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
        // title update's layout (each chunk inline), which the disc build read as garbage meshes (huge triangles over the
        // sky, a crash reading their vertices): converted to the disc's layout, or, failing that, one of the disc's
        // river boats in their place
        char magic[4] = {};
        std::ifstream(e.path(), std::ios::binary).read(magic, 4);
        if (std::memcmp(magic, "INDX", 4) != 0) {
            std::ifstream f(e.path(), std::ios::binary);
            std::vector<uint8_t> in((std::istreambuf_iterator<char>(f)), {});
            std::vector<uint8_t> out = ConvertPak(in);
            if (out.empty()) {
                static const char* kBoats[] = {"StockholmBoat1.pak", "boatBig.pak"};
                static int next = 0;
                const char* boat = kBoats[next++ % 2];
                if (kernel::ReadGameFile(std::string("D:\\Cache\\Objects\\") + boat, out))
                    LOGR("pgr2: %s from %s could not be converted: the disc's %s stands in", name.c_str(), folder.c_str(), boat);
            } else {
                LOGR("pgr2: %s from %s converted from the title update's pak layout", name.c_str(), folder.c_str());
            }
            if (!out.empty())
                std::ofstream(dest, std::ios::binary).write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size()));
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

// PGR2: a car pack loaded (0x157180: metadata, pack folder, ...; 1 when loaded; each new car's record keeps the
// folder). Where the disc's cars' files are found too: the models in the cache, Z:\Cars (copied there from
// D:\Cache\Cars at boot), the handling in D:\CarDynamics, the engine sounds in D:\data\audio; the pack's are put there
// as well (copied, linked), where the disc has none of that name
X86_EXTERN_FUNC(__imp__sub_00157180);
X86_FUNC(sub_00157180) {
    uint32_t folder_va = kernel::R32(ctx.esp + 8);
    __imp__sub_00157180(ctx, base);
    if (!On() || !folder_va) return;
    std::string folder;
    for (uint32_t p = folder_va; kernel::G<uint8_t>(p) && folder.size() < 260; p++) folder += char(kernel::G<uint8_t>(p));
    LOGR("pgr2: car pack %s %s", folder.c_str(), ctx.eax ? "loaded" : "refused");
    if (!ctx.eax) return;
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path from = kernel::ResolvePath(folder, 0), cars = kernel::ResolvePath("Z:\\Cars", 0);
    if (from.empty() || cars.empty()) return;
    int models = 0, links = 0;
    for (const auto& e : fs::directory_iterator(from, ec)) {
        if (!e.is_regular_file(ec)) continue;
        std::string name = e.path().filename().string(), lower = name;
        for (char& c : lower) c = char(std::tolower(static_cast<unsigned char>(c)));
        std::string ext = e.path().extension().string();
        for (char& c : ext) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (ext.rfind(".pak", 0) == 0) {
            fs::path dest = cars / name;
            if (!(fs::exists(dest, ec) && fs::file_size(dest, ec) == e.file_size(ec)))
                fs::copy_file(e.path(), dest, fs::copy_options::overwrite_existing, ec), models++;
            continue;
        }
        std::string where;
        if (ext == ".ini" && lower.rfind("dcontentcar", 0) == 0) where = "D:\\CarDynamics\\";
        else if (ext == ".dat" || ext == ".xwb") where = "D:\\data\\audio\\";
        if (where.empty() || kernel::GameFileExists(where + name)) continue;
        kernel::CreateSymbolicLink("\\??\\" + where + name, "\\??\\" + folder + "\\" + name);
        links++;
    }
    LOGR("pgr2: car pack %s: %d models copied to Z:\\Cars, %d handling and sound files linked", folder.c_str(), models, links);
}
