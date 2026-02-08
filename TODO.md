# CANnect — TODO & Issues

## 🔴 Critical — Memory Safety & Correctness

- [X] **Fix memory leaks in `Cannect::run()`** — `new SocketCanTransport`, `CanLogger`, `CanTsProtocol`, and `CanSender` are never deleted. Use `std::unique_ptr` for all owned pointers (`socket`, `observer`, `protocol`).
- [ ] **Replace busy-wait spin loop** — `while (listener.isRunning());` in `Cannect::run()` burns 100% CPU. Use `std::condition_variable`, a signal handler, or at minimum `std::this_thread::sleep_for()`.
- [ ] **Add read timeout / `poll()` to `SocketCanTransport::readFrame()`** — Blocking `::read()` prevents `CanListener::stop()` from working. Use `setsockopt(SO_RCVTIMEO)`, `poll()`/`select()`, or `shutdown()` to unblock.
- [X] **Check `socket->open()` return value** — If `open()` fails, execution continues and uses fd = -1, causing undefined behavior.
- [X] **Check `ArgumentParser::parse()` return value** — `parse()` returns `false` on `--help` or errors, but the return value is ignored in `Cannect::run()`.
- [ ] **`CanTsProtocol::send()` is a no-op** — The method receives data but does nothing with it.

---

## 🟠 High — Design & Architecture

- [X] **Use `std::unique_ptr` for ownership** — `Cannect` holds `ICanTransport*`, `ICanObserver*`, `IProtocol*` as raw owning pointers. Use smart pointers to express ownership and guarantee cleanup.
- [ ] **Fix leaky Logger singleton** — `Logger::getInstance()` does `new Logger()` but never deletes. Use Meyer's Singleton (`static Logger instance;`) or `std::unique_ptr`.
- [ ] **Implement or remove `CanCsvLogger` and `CanJsonLogger`** — Both `.cpp` files are empty. Either implement them or remove them.
- [ ] **Add signal handling (SIGINT/SIGTERM)** — No way to cleanly stop the application. Add a handler that calls `listener.stop()`.
- [X] **Simplify or justify `CanDispatcher` frame storage** — `addFrame()` + `notify()` is always called in sequence, making the `canFrames` vector unnecessary overhead.
- [X] **Observer lifetime safety** — `CanDispatcher` stores `ICanObserver*` with no lifetime guarantees. Consider `std::weak_ptr` or document ownership semantics.

---

## 🟡 Medium — Code Quality & Robustness

- [X] **Fix README/code argument mismatch** — README says `--iface`, code registers `--can-iface`.
- [X] **Fix `--version` help text** — Currently says "Verbose output (repeatable)" (copy-paste error).
- [ ] **Remove or mark unimplemented CLI arguments** — `--listen`, `--connect`, `--bidir`, `--speed`, `--loop`, `--period`, `--count` are registered but never used.
- [X] **Make `ICanObserver::update()` take `const CanFrame&`** — Observers should not modify the frame.
- [X] **Fix double-newline in `CanFrame::operator<<`** — The operator adds `std::endl`, and callers also add `std::endl`.
- [X] **Fix `install`/`uninstall` Makefile targets** — They reference undefined `$(TARGET)` instead of `$(EXE_TARGET)`.
- [X] **Add `.clang-format` config** — `make format` exists but no config file, so formatting depends on user defaults.
- [X] **Standardize header include paths** — `Cannect.hpp` uses `"cli/ArgumentParser.hpp"` (relative) while others use `"cannect/core/..."` (project-root-relative).

---

## 🔵 Low — Testing, Build & Project Hygiene

- [ ] **Write real unit tests** — Current tests are trivial (1+1=2, true is true). No tests for: `CanFrame`, `ArgumentParser`, `CanDispatcher`, `EncoderDecoder`, `CanFilter`.
- [ ] **Create `MockCanTransport`** — Needed to unit test `CanListener`, `CanSender`, and `Cannect` without hardware.
- [ ] **Add `EncoderDecoder` roundtrip tests** — Encode → decode should return the original header.
- [ ] **Add `ArgumentParser` edge-case tests** — Missing values, unknown args, type mismatches.
- [ ] **Consider migrating to Google Test or Catch2** — Custom framework lacks fixtures, `EXPECT_THROW`, setup/teardown, test filtering, XML output.
- [ ] **Add `.gitignore`** — Exclude `build/`, `compile_commands.json`, `*.o`, `*.d`, `*.gcda`, `*.gcno`.
- [ ] **Remove `compile_commands.json` from repo** — Generated file should be gitignored.
- [ ] **Add CI/CD pipeline** — No `.github/workflows/` or equivalent. Automate build + test + coverage.
- [ ] **Consider CMake** — Raw Makefiles don't scale. CMake improves IDE integration, dependency management, and cross-platform support.
- [ ] **Add a `LICENSE` file** — Required if the project is intended to be open source.

---

## 💡 Feature Gaps (declared in README/CLI but not implemented)

- [ ] **Bridge mode** (`--iface-a` / `--iface-b`) — Arguments registered but never consumed.
- [ ] **Record to file** (`--output`) — Creates `CanLogger` but only prints to stdout, doesn't write to file.
- [ ] **Replay mode** (`--input`, `--speed`, `--loop`) — Not implemented.
- [ ] **Filter mode** (`--filter`) — `setFilters()` exists but is never called from CLI.
- [ ] **Send mode** (`--id`, `--data`, `--period`, `--count`) — Not implemented.
- [ ] **Decode display** (`--decode`) — Not implemented.
- [ ] **Network forwarding** (`--listen`, `--connect`, `--bidir`) — Not implemented.
- [ ] **`CanTsProtocol::send()`** — Method body is empty.
