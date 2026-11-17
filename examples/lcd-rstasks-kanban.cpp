/*
 * lcd-rstasks-kanban.cpp - Kanban board viewer for RsTask workspaces
 *
 * Renders the RsTask kanban board (queue / work / review / done) on an
 * LCD2USB-connected character LCD. The board is fetched by running the
 * `rstasks` CLI (`rstasks board`) against a workspace database.
 *
 * Row 0 always shows the summary: column counts Q/W/R/D, plus a page
 * marker and clock when they fit. Task lines are prefixed with a per-task
 * custom character: a deterministic 5x8 "identicon" derived from the task
 * id (mirrored for symmetry), whose two bottom rows mark the lane
 * (queue = bottom corners, work = up arrow, review = diamond), uploaded
 * to the display's CGRAM.
 *
 * Layout of the remaining rows:
 *   default      active tasks (work, review, queue) paginated; the page
 *                flips on every data refresh
 *   --details    "horse racing" scrolling, only for rows whose content
 *                does not fit the screen (a fitting row stays static):
 *                  rows 2-3  all active tasks stream upward through the
 *                            detail rows, one task per step
 *                  row 4     one lane per row (WORK | REVIEW | QUEUE);
 *                            each lane's tasks marquee horizontally,
 *                            icons riding along
 *
 * Data refreshes every --duration minutes; the horse-racing animation
 * runs on its own ~4 fps clock between refreshes.
 *
 * Usage: ./lcd_rstasks_kanban [--rstasks-path PATH]... [--rows N] [--width N]
 *                             [--duration MIN] [--details] [--once] [--dry-run]
 *
 * Options:
 *   --rstasks-path PATH  RsTask workspace directory (holding .rstask/) or a
 *                        direct path to a rstask.sqlite file. Repeatable;
 *                        the first path that exists is used. Default: "."
 *   --rows N             LCD row count (default 4)
 *   --width N            LCD column count (default 20)
 *   --duration MIN       Minutes between board refreshes (default 10)
 *   --details            horse-racing detail view (see above)
 *   --once               Render a single frame and exit
 *   --dry-run            Print frames to stdout instead of driving the LCD
 *   --help               Show this help
 */

#include "lcd2usb/lcd2usb_lib.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace LCD2USB;

namespace {

volatile sig_atomic_t running = 1;

void onSignal(int) {
    running = 0;
}

// Animation clock: one horse-racing step every 250 ms between data refreshes.
constexpr auto kFrame = std::chrono::milliseconds(250);
// Vertical slide advances one task every 6th frame (1.5 s per step).
constexpr uint64_t kSlideEvery = 6;
// Dry-run prints at most one frame per second so the terminal is not flooded.
constexpr uint64_t kDryRunPrintEvery = 4;

struct Task {
    char lane;  // 'Q' | 'W' | 'R'
    std::string id;
    int priority = 0;
    std::string title;
};

struct Board {
    std::array<int, 4> count = {0, 0, 0, 0};          // queue, work, review, done
    std::array<std::vector<Task>, 4> lanes;           // tasks per lane
    std::vector<Task> active;                         // work + review + queue
};

int laneIndex(const std::string& status) {
    if (status == "queue") return 0;
    if (status == "work") return 1;
    if (status == "review") return 2;
    if (status == "done") return 3;
    return -1;
}

char laneLetter(int lane) {
    return "QWRD"[lane];
}

// Pad with spaces or truncate so the line is exactly `width` characters;
// the HD44780 cursor wraps to the next row if we send more than width bytes.
std::string fit(const std::string& s, size_t width) {
    if (s.size() > width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

bool runCommand(const std::string& cmd, std::string& out) {
    out.clear();
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buf[512];
    while (size_t n = fread(buf, 1, sizeof(buf), pipe)) {
        out.append(buf, n);
    }
    return pclose(pipe) == 0;
}

std::string shellQuote(const std::string& s) {
    std::string q = "'";
    for (char c : s) {
        if (c == '\'') q += "'\\''";
        else q += c;
    }
    q += "'";
    return q;
}

// A candidate may be a workspace directory (uses .rstask/rstask.sqlite
// inside) or a direct path to the sqlite file.
bool resolveDatabase(const std::vector<std::string>& candidates, std::string& db) {
    std::error_code ec;
    for (const auto& candidate : candidates) {
        std::filesystem::path p(candidate);
        if (std::filesystem::is_directory(p, ec)) {
            p /= std::filesystem::path(".rstask") / "rstask.sqlite";
        }
        if (std::filesystem::is_regular_file(p, ec)) {
            db = p.string();
            return true;
        }
    }
    return false;
}

// Parse `rstasks board` output:
//   == queue (2) ==
//   abcd1234  queue    3     Some title
bool parseBoard(const std::string& text, Board& board) {
    board = Board{};
    std::istringstream in(text);
    std::string line;
    int lane = -1;
    bool sawSection = false;
    while (std::getline(in, line)) {
        if (line.rfind("==", 0) == 0) {
            const auto open = line.find('(');
            const auto close = line.find(')', open == std::string::npos ? 0 : open + 1);
            if (open == std::string::npos || close == std::string::npos) continue;
            std::string name = line.substr(2, open - 2);
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            lane = laneIndex(name);
            if (lane >= 0) {
                sawSection = true;
                board.count[lane] = std::atoi(line.substr(open + 1, close - open - 1).c_str());
            }
            continue;
        }
        if (lane < 0 || lane == 3) continue;   // header or done section
        if (line.empty() || line[0] == '(') continue;  // "(no tasks)"
        std::istringstream iss(line);
        std::string id, status, prio;
        if (!(iss >> id >> status >> prio)) continue;
        if (id == "id") continue;  // column header row
        std::string title;
        std::getline(iss, title);
        title.erase(0, title.find_first_not_of(" \t"));  // npos -> empty
        board.lanes[lane].push_back(Task{laneLetter(lane), id, std::atoi(prio.c_str()), title});
    }
    // Display order: work, review, then queue (workflow order, done excluded).
    board.active = board.lanes[1];
    board.active.insert(board.active.end(), board.lanes[2].begin(), board.lanes[2].end());
    board.active.insert(board.active.end(), board.lanes[0].begin(), board.lanes[0].end());
    return sawSection;
}

// Deterministic 5x8 "identicon" for a task: FNV-1a of the task id, six rows
// of a mirrored random-looking pattern, and a two-row lane footer so the
// icon alone still tells the workflow state.
std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT> taskGlyph(const std::string& id, char lane) {
    CustomChar cc;
    cc.clear();
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : id) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    h ^= static_cast<uint64_t>(static_cast<unsigned char>(lane)) << 56;
    int bit = 0;
    for (int y = 0; y < 6; ++y) {
        for (int x = 0; x < 3; ++x, ++bit) {
            if ((h >> bit) & 1ULL) {
                cc.setPixel(x, y, true);
                cc.setPixel(LCD::CUSTOM_CHAR_WIDTH - 1 - x, y, true);
            }
        }
    }
    const auto footer = [&](int y, unsigned pattern) {
        for (int x = 0; x < LCD::CUSTOM_CHAR_WIDTH; ++x) {
            cc.setPixel(x, y, (pattern >> x) & 1U);
        }
    };
    switch (lane) {
        case 'Q': footer(7, 0x11); break;                   // parked: bottom corners
        case 'W': footer(6, 0x04); footer(7, 0x0E); break;  // in progress: up arrow
        case 'R': footer(6, 0x0E); footer(7, 0x11); break;  // in review: diamond
        default: break;
    }
    return cc.toBytes();
}

// Assigns CGRAM slots to the tasks used by one frame, in order of first
// appearance. Slot 0 is skipped (unreliable on many HD44780 variants); when
// the frame needs more distinct icons than the 7 usable slots, the extras
// fall back to a plain space.
struct IconBook {
    std::vector<const Task*> tasks;
    std::array<std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>, LCD::USABLE_SLOT_COUNT> glyphs{};

    char icon(const Task& t) {
        for (size_t i = 0; i < tasks.size(); ++i) {
            if (tasks[i] == &t) return static_cast<char>(LCD::FIRST_USABLE_SLOT + i);
        }
        if (tasks.size() >= static_cast<size_t>(LCD::USABLE_SLOT_COUNT)) return ' ';
        tasks.push_back(&t);
        glyphs[tasks.size() - 1] = taskGlyph(t.id, t.lane);
        return static_cast<char>(LCD::FIRST_USABLE_SLOT + tasks.size() - 1);
    }
};

// One rendered frame: the character cell lines plus the CGRAM images they
// reference. Lines may embed bytes 0x01-0x07 addressing the CGRAM slots.
struct Frame {
    std::vector<std::string> lines;
    IconBook book;
};

// "<icon> <id4> <title>" — the icon replaces the old lane-letter column.
std::string taskInline(const Task& t, IconBook& book) {
    std::string s;
    s += book.icon(t);
    s += ' ';
    s += t.id.substr(0, 4);
    s += ' ';
    s += t.title;
    return s;
}

// Summary row: column counts, right-aligned page marker (paged mode only)
// and clock, each only when it fits after the counts.
std::string buildHeader(const Board& board, int width, size_t pages, size_t page) {
    char counts[64];
    std::snprintf(counts, sizeof(counts), "Q:%d W:%d R:%d D:%d",
                  board.count[0], board.count[1], board.count[2], board.count[3]);

    std::time_t now = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&now, &tmv);
    char clock[8];
    std::snprintf(clock, sizeof(clock), "%02d:%02d", tmv.tm_hour, tmv.tm_min);

    std::string right;
    if (pages > 1) {
        char pg[16];
        std::snprintf(pg, sizeof(pg), "p%zu/%zu", page + 1, pages);
        right = pg;
        if (std::string(counts).size() + 1 + 5 + 1 + right.size() <= static_cast<size_t>(width)) {
            right = std::string(clock) + " " + right;
        }
    } else if (std::string(counts).size() + 1 + 5 <= static_cast<size_t>(width)) {
        right = clock;
    }
    std::string header = counts;
    if (!right.empty() && header.size() + 1 + right.size() <= static_cast<size_t>(width)) {
        header += std::string(static_cast<size_t>(width) - header.size() - right.size(), ' ');
        header += right;
    }
    return fit(header, static_cast<size_t>(width));
}

// Default mode: paginated task list, one page per data refresh.
Frame renderPaged(const Board& board, int rows, int width, size_t tick) {
    const size_t taskRows = rows > 1 ? static_cast<size_t>(rows - 1) : 0;
    const size_t pages = taskRows == 0
                             ? 1
                             : std::max<size_t>(1, (board.active.size() + taskRows - 1) / taskRows);

    Frame frame;
    frame.lines.push_back(buildHeader(board, width, pages, tick % pages));
    if (taskRows == 0) return frame;

    if (board.active.empty()) {
        const char* msg = board.count[3] > 0 ? "-- all done! --" : "-- board empty --";
        frame.lines.push_back(fit(msg, static_cast<size_t>(width)));
    } else {
        const size_t start = (tick % pages) * taskRows;
        for (size_t i = start; i < board.active.size() && frame.lines.size() < static_cast<size_t>(rows); ++i) {
            frame.lines.push_back(fit(taskInline(board.active[i], frame.book),
                                      static_cast<size_t>(width)));
        }
    }
    while (frame.lines.size() < static_cast<size_t>(rows)) {
        frame.lines.emplace_back(static_cast<size_t>(width), ' ');
    }
    return frame;
}

// One lane row for the 4-row details layout: the lane's tasks joined into a
// single string that marquees horizontally (horse racing) with the per-task
// icons riding along. A lane whose text fits the row stays static.
std::string laneRow(char lane, const std::vector<Task>& items, int width,
                    uint64_t anim, IconBook& book) {
    const std::string prefix = std::string(1, lane) + " ";
    const size_t budget = width > static_cast<int>(prefix.size())
                              ? static_cast<size_t>(width) - prefix.size()
                              : 0;
    if (items.empty()) return fit(prefix + "- none -", static_cast<size_t>(width));

    std::string text;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) text += ' ';
        text += taskInline(items[i], book);
    }
    if (text.size() <= budget) return fit(prefix + text, static_cast<size_t>(width));

    // Longer than the row: scroll with a 2-space gap so the wrap reads cleanly.
    const std::string loop = text + "  ";
    const size_t off = anim % loop.size();
    std::string shown = loop.substr(off, budget);
    if (shown.size() < budget) shown += loop.substr(0, budget - shown.size());
    return fit(prefix + shown, static_cast<size_t>(width));
}

// --details mode. anim is the animation frame counter (4 fps).
Frame renderDetails(const Board& board, int rows, int width, uint64_t anim) {
    Frame frame;
    frame.lines.push_back(buildHeader(board, width, 1, 0));
    const int taskRows = rows - 1;
    if (taskRows <= 0) return frame;

    if (rows == 4) {
        // One lane per row: WORK | REVIEW | QUEUE.
        frame.lines.push_back(laneRow('W', board.lanes[1], width, anim, frame.book));
        frame.lines.push_back(laneRow('R', board.lanes[2], width, anim, frame.book));
        frame.lines.push_back(laneRow('Q', board.lanes[0], width, anim, frame.book));
        return frame;
    }

    // Rows 2-3: all active tasks stream upward through the detail rows.
    // When everything fits, the list stands still — no racing needed.
    if (board.active.empty()) {
        const char* msg = board.count[3] > 0 ? "-- all done! --" : "-- board empty --";
        frame.lines.push_back(fit(msg, static_cast<size_t>(width)));
    } else {
        const bool fits = board.active.size() <= static_cast<size_t>(taskRows);
        const size_t off = fits ? 0 : (anim / kSlideEvery) % board.active.size();
        for (int i = 0; i < taskRows; ++i) {
            const Task& t = board.active[(off + i) % board.active.size()];
            frame.lines.push_back(fit(taskInline(t, frame.book),
                                      static_cast<size_t>(width)));
        }
    }
    while (frame.lines.size() < static_cast<size_t>(rows)) {
        frame.lines.emplace_back(static_cast<size_t>(width), ' ');
    }
    return frame;
}

// Remembers what is already in CGRAM so unchanged glyphs are not re-sent
// on every 250 ms animation frame.
struct CgramCache {
    std::array<std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>, LCD::MAX_CUSTOM_CHARS> last{};
    std::array<bool, LCD::MAX_CUSTOM_CHARS> valid{};
};

void drawFrame(LCDDisplay& lcd, CgramCache& cache, const Frame& frame) {
    for (size_t i = 0; i < frame.book.tasks.size(); ++i) {
        const int slot = LCD::FIRST_USABLE_SLOT + static_cast<int>(i);
        const auto& glyph = frame.book.glyphs[i];
        if (!cache.valid[slot] || cache.last[slot] != glyph) {
            lcd.uploadCustomCharacter(slot, glyph);
            cache.last[slot] = glyph;
            cache.valid[slot] = true;
        }
    }
    for (size_t i = 0; i < frame.lines.size(); ++i) {
        lcd.writeToLine(static_cast<int>(i), frame.lines[i]);
    }
}

void printFrame(const Frame& frame) {
    // CGRAM bytes are control characters on a terminal; show a placeholder.
    const std::string bar(static_cast<size_t>(frame.lines.front().size()), '-');
    std::cout << "+" << bar << "+\n";
    for (const auto& line : frame.lines) {
        std::cout << "|";
        for (char c : line) {
            std::cout << (static_cast<unsigned char>(c) < 0x08 ? '#' : c);
        }
        std::cout << "|\n";
    }
    std::cout << "+" << bar << "+\n" << std::flush;
}

void usage(const char* argv0) {
    std::cout << "Usage: " << argv0
              << " [--rstasks-path PATH]... [--rows N] [--width N]\n"
                 "                             [--duration MIN] [--details] [--once] [--dry-run]\n"
                 "\n"
                 "  --rstasks-path PATH  RsTask workspace dir (with .rstask/) or path to\n"
                 "                       rstask.sqlite. Repeatable; first match wins.\n"
                 "  --rows N             LCD rows (default 4)\n"
                 "  --width N            LCD columns (default 20)\n"
                 "  --duration MIN       minutes between board refreshes (default 10)\n"
                 "  --details            horse-racing details for rows that overflow\n"
                 "                       (fitting rows stay static): rows 2-3 stream\n"
                 "                       all active tasks upward; row 4 shows\n"
                 "                       WORK|REVIEW|QUEUE, one lane per row\n"
                 "  --once               render one frame and exit\n"
                 "  --dry-run            print frames to stdout, do not touch the LCD\n"
                 "  --help               show this help\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> candidates;
    int rows = 4;
    int width = 20;
    double durationMin = 10.0;
    bool details = false;
    bool once = false;
    bool dryRun = false;

    static const struct option kOpts[] = {
        {"rstasks-path", required_argument, nullptr, 'p'},
        {"rows", required_argument, nullptr, 'r'},
        {"width", required_argument, nullptr, 'w'},
        {"duration", required_argument, nullptr, 'd'},
        {"details", no_argument, nullptr, 'D'},
        {"once", no_argument, nullptr, 'o'},
        {"dry-run", no_argument, nullptr, 'n'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
    };
    int ch;
    while ((ch = getopt_long(argc, argv, "p:r:w:d:Donh", kOpts, nullptr)) != -1) {
        switch (ch) {
            case 'p': candidates.push_back(optarg); break;
            case 'r': rows = std::max(1, std::atoi(optarg)); break;
            case 'w': width = std::max(4, std::atoi(optarg)); break;
            case 'd': durationMin = std::atof(optarg); break;
            case 'D': details = true; break;
            case 'o': once = true; break;
            case 'n': dryRun = true; break;
            case 'h': usage(argv[0]); return 0;
            default: usage(argv[0]); return 2;
        }
    }
    if (candidates.empty()) candidates.push_back(".");
    durationMin = std::max(0.01, durationMin);  // >= 0.6 s, keeps --duration testable

    std::string db;
    if (!resolveDatabase(candidates, db)) {
        std::cerr << "error: no rstask.sqlite found in:";
        for (const auto& c : candidates) std::cerr << " " << c;
        std::cerr << "\n(initialize a workspace with `rstasks init` first)" << std::endl;
        return 2;
    }
    const std::string cmd = "rstasks --db " + shellQuote(db) + " board 2>/dev/null";

    Board board;
    auto fetch = [&]() {
        std::string out;
        return runCommand(cmd, out) && parseBoard(out, board);
    };
    if (!fetch()) {
        std::cerr << "error: `rstasks board` failed for db " << db
                  << " (is the rstasks binary installed and on PATH?)" << std::endl;
        return 2;
    }

    LCDDisplay display;
    bool haveLcd = false;
    if (!dryRun) {
        haveLcd = display.initialize();
        if (haveLcd) {
            display.setContrast(200);
            display.setBrightness(255);
        } else {
            std::cerr << "warning: no LCD2USB device found; "
                         "falling back to console output" << std::endl;
        }
    }

    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    const auto duration = std::chrono::milliseconds(
        static_cast<long long>(durationMin * 60'000));
    auto nextRefresh = std::chrono::steady_clock::now() + duration;
    CgramCache cgram;
    uint64_t dataTick = 0;
    uint64_t anim = 0;
    while (running) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextRefresh) {
            if (!fetch()) {
                std::cerr << "warning: board refresh failed; keeping last frame" << std::endl;
            } else {
                ++dataTick;
            }
            nextRefresh = now + duration;
        }
        Frame frame = details ? renderDetails(board, rows, width, anim)
                              : renderPaged(board, rows, width, dataTick);
        if (haveLcd) {
            drawFrame(display, cgram, frame);
        } else if (once || anim % kDryRunPrintEvery == 0) {
            printFrame(frame);
        }
        if (once) break;
        ++anim;
        for (auto slept = std::chrono::milliseconds(0);
             slept < kFrame && running;
             slept += std::chrono::milliseconds(50)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // Leave the board on the screen after a single-shot render; only clear
    // when a long-running session is interrupted.
    if (haveLcd && !once) display.clear();
    return 0;
}
