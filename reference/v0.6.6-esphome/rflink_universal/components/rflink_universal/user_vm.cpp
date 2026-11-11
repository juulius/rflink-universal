#include "user_vm.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace esphome {
namespace rflink_universal {
namespace user_vm {
namespace {

constexpr size_t MAX_STEPS = 2048;
constexpr size_t MAX_STACK = 32;

bool number(const std::string &text, int64_t &result) {
  if (text.empty() || text.size() > 16) return false;
  char *end = nullptr;
  errno = 0;
  const long long value = std::strtoll(text.c_str(), &end, 0);
  if (errno || end != text.c_str() + text.size() || value < INT32_MIN || value > UINT32_MAX) return false;
  result = value;
  return true;
}

bool pop(std::vector<int64_t> &stack, int64_t &value) {
  if (stack.empty()) return false;
  value = stack.back(); stack.pop_back(); return true;
}

bool push(std::vector<int64_t> &stack, int64_t value) {
  if (stack.size() >= MAX_STACK) return false;
  stack.push_back(value); return true;
}

bool valid_opcode(const std::string &op, Mode mode, unsigned &arguments) {
  if (op == "CONST" || op == "BYTE" || op == "PULSE" || op == "JMP" || op == "JZ" ||
      op == "ACTION" || op == "ACTEQ" || op == "STATE" || op == "BUMP" ||
      op == "REPEATS" || op == "MARKUS" || op == "SPACEUS" || op == "BYTEVAL") {
    arguments = 1;
    if (op == "ACTION") return mode == Mode::RX;
    if (op == "ACTEQ" || op == "BUMP" || op == "REPEATS" || op == "MARKUS" ||
        op == "SPACEUS" || op == "BYTEVAL") return mode == Mode::TX;
    return true;
  }
  if (op == "MATCHRAW" || op == "MATCHBYTE") { arguments = 2; return mode == Mode::RX; }
  if (op == "FIELD") { arguments = 2; return mode == Mode::RX; }
  if (op == "LEN" || op == "ADD" || op == "SUB" || op == "AND" || op == "OR" ||
      op == "XOR" || op == "EQ" || op == "LT" || op == "GT" || op == "SHL" ||
      op == "SHR" || op == "DUP" || op == "DROP" || op == "REQUIRE" ||
      op == "ABS" || op == "NOT" || op == "HALT" || op == "ID") { arguments = 0; return true; }
  if (mode == Mode::RX && op == "ACCEPT") { arguments = 0; return true; }
  if (mode == Mode::TX && (op == "MARK" || op == "SPACE" || op == "EMIT_BYTE" || op == "SEND")) {
    arguments = 0; return true;
  }
  return false;
}

bool execute(const Program &program, Mode mode, const RFFrame *rx, const std::string &action,
             uint8_t radio, const std::map<std::string, uint32_t> &state,
             DecodeResult *decoded, TxFrame *tx, std::string &error) {
  if ((mode == Mode::RX) != (rx != nullptr) || (mode == Mode::TX) != (tx != nullptr)) {
    error = "wrong program mode"; return false;
  }
  std::vector<int64_t> stack;
  stack.reserve(MAX_STACK);
  size_t ip = 0, steps = 0;
  while (ip < program.code.size() && ++steps <= MAX_STEPS) {
    const auto &ins = program.code[ip];
    const auto &op = ins.op;
    int64_t a = 0, b = 0;
    if (op == "CONST") {
      if (!number(ins.arg, a) || !push(stack, a)) break;
    } else if (op == "LEN") {
      if (!push(stack, rx ? (rx->kind == FrameKind::RAW && rx->raw ? rx->raw->size() :
                               rx->kind == FrameKind::PACKET && rx->packet ? rx->packet->size() : 0) : 0)) break;
    } else if (op == "BYTE" || op == "PULSE") {
      if (!rx || !number(ins.arg, a) || a < 0 ||
          (op == "BYTE" && (rx->kind != FrameKind::PACKET || !rx->packet || size_t(a) >= rx->packet->size())) ||
          (op == "PULSE" && (rx->kind != FrameKind::RAW || !rx->raw || size_t(a) >= rx->raw->size()))) break;
      if (!push(stack, op == "BYTE" ? (*rx->packet)[size_t(a)] : (*rx->raw)[size_t(a)])) break;
    } else if (op == "MATCHRAW" || op == "MATCHBYTE") {
      if (!rx || !number(ins.arg, a) || !number(ins.extra, b) || a < 0) break;
      bool equal = false;
      if (op == "MATCHRAW" && rx->kind == FrameKind::RAW && rx->raw && size_t(a) < rx->raw->size()) {
        const int64_t sample = (*rx->raw)[size_t(a)];
        const int64_t tolerance = std::max<int64_t>(100, std::llabs(b) / 4);
        equal = (sample < 0) == (b < 0) && std::llabs(sample - b) <= tolerance;
      }
      if (op == "MATCHBYTE" && rx->kind == FrameKind::PACKET && rx->packet && size_t(a) < rx->packet->size())
        equal = b >= 0 && b <= 255 && (*rx->packet)[size_t(a)] == b;
      if (!push(stack, equal ? 1 : 0)) break;
    } else if (op == "DUP") {
      if (stack.empty() || !push(stack, stack.back())) break;
    } else if (op == "DROP") {
      if (!pop(stack, a)) break;
    } else if (op == "ABS" || op == "NOT") {
      if (!pop(stack, a) || !push(stack, op == "ABS" ? (a < 0 ? -a : a) : (a == 0 ? 1 : 0))) break;
    } else if (op == "ADD" || op == "SUB" || op == "AND" || op == "OR" || op == "XOR" ||
               op == "EQ" || op == "LT" || op == "GT" || op == "SHL" || op == "SHR") {
      if (!pop(stack, b) || !pop(stack, a)) break;
      int64_t value = 0;
      if (op == "ADD") value = a + b;
      else if (op == "SUB") value = a - b;
      else if (op == "AND") value = a & b;
      else if (op == "OR") value = a | b;
      else if (op == "XOR") value = a ^ b;
      else if (op == "EQ") value = a == b;
      else if (op == "LT") value = a < b;
      else if (op == "GT") value = a > b;
      else if (op == "SHL") { if (b < 0 || b > 31) break; value = int64_t(uint32_t(a) << b); }
      else { if (b < 0 || b > 31) break; value = int64_t(uint32_t(a) >> b); }
      if (value < INT32_MIN || value > UINT32_MAX || !push(stack, value)) break;
    } else if (op == "JMP" || op == "JZ") {
      if (!number(ins.arg, a) || a < 0 || size_t(a) >= program.code.size()) break;
      if (op == "JZ" && !pop(stack, b)) break;
      if (op == "JMP" || b == 0) { ip = size_t(a); continue; }
    } else if (op == "REQUIRE") {
      if (!pop(stack, a) || a == 0) { error = "frame did not satisfy decoder rule"; return false; }
    } else if (op == "ID") {
      if (!decoded || !pop(stack, a) || a < 0) break;
      if (!decoded->identity_key.empty()) decoded->identity_key += ':';
      std::ostringstream hex;
      hex << std::hex << std::uppercase << a;
      decoded->identity_key += hex.str();
    } else if (op == "FIELD") {
      if (!decoded || !pop(stack, a) || decoded->fields.size() >= 16) break;
      decoded->fields.push_back({ins.arg, std::to_string(a), ins.extra == "-" ? "" : ins.extra});
    } else if (op == "ACTION") {
      if (!decoded) break;
      decoded->action = ins.arg;
    } else if (op == "ACTEQ") {
      if (!push(stack, action == ins.arg ? 1 : 0)) break;
    } else if (op == "STATE") {
      const auto it = state.find(ins.arg);
      const auto next = tx ? tx->next_state.find(ins.arg) : std::map<std::string, uint32_t>::const_iterator{};
      if (!push(stack, tx && next != tx->next_state.end() ? next->second : it == state.end() ? 0 : it->second)) break;
    } else if (op == "BUMP") {
      if (!tx) break;
      const auto it = tx->next_state.find(ins.arg);
      const auto old = state.find(ins.arg);
      const uint32_t current = it != tx->next_state.end() ? it->second : old != state.end() ? old->second : 0;
      if (current == UINT32_MAX || tx->next_state.size() >= 8) break;
      tx->next_state[ins.arg] = current + 1;
      if (!push(stack, current + 1)) break;
    } else if (op == "REPEATS") {
      if (!tx || !number(ins.arg, a) || a < 1 || a > 10) break;
      tx->repeats = uint8_t(a);
    } else if (op == "MARKUS" || op == "SPACEUS") {
      if (!tx || !number(ins.arg, a) || a < 50 || a > 1000000 || tx->raw.size() >= 512 || !tx->packet.empty()) break;
      tx->raw.push_back(op == "MARKUS" ? int32_t(a) : -int32_t(a));
    } else if (op == "BYTEVAL") {
      if (!tx || !number(ins.arg, a) || a < 0 || a > 255 || tx->packet.size() >= 64 || !tx->raw.empty()) break;
      tx->packet.push_back(uint8_t(a));
    } else if (op == "MARK" || op == "SPACE") {
      if (!tx || !pop(stack, a) || a < 50 || a > 1000000 || tx->raw.size() >= 512 || !tx->packet.empty()) break;
      tx->raw.push_back(op == "MARK" ? int32_t(a) : -int32_t(a));
    } else if (op == "EMIT_BYTE") {
      if (!tx || !pop(stack, a) || a < 0 || a > 255 || tx->packet.size() >= 64 || !tx->raw.empty()) break;
      tx->packet.push_back(uint8_t(a));
    } else if (op == "ACCEPT") {
      if (!decoded || decoded->identity_key.empty() || (decoded->action.empty() && decoded->fields.empty())) break;
      decoded->valid = true;
      decoded->identity_confidence = IdentityConfidence::MEDIUM;
      return true;
    } else if (op == "SEND") {
      if (!tx || (tx->raw.empty() && tx->packet.empty()) ||
          (!tx->raw.empty() && (tx->raw.front() < 0 || tx->raw.size() % 2 != 0))) break;
      tx->radio = radio;
      return true;
    } else if (op == "HALT") {
      error = "program stopped without accepting";
      return false;
    } else { break; }
    ++ip;
  }
  error = steps > MAX_STEPS ? "instruction limit exceeded" : "invalid instruction, frame or output";
  return false;
}

}  // namespace

bool identifier(const std::string &value) {
  if (value.empty() || value.size() > 32 || !((value[0] >= 'a' && value[0] <= 'z'))) return false;
  for (char c : value)
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
  return true;
}

bool compile(const std::string &source, Mode mode, Program &out, std::string &error) {
  out.code.clear();
  if (source.empty() || source.size() > MAX_SOURCE) { error = "program must be 1-4096 bytes"; return false; }
  std::istringstream input(source);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream parts(line);
    Instruction ins;
    std::string trailing;
    parts >> ins.op >> ins.arg >> ins.extra;
    parts >> trailing;
    unsigned argc = 0;
    if (ins.op.empty() || !valid_opcode(ins.op, mode, argc) || !trailing.empty() ||
        (argc == 0 && (!ins.arg.empty() || !ins.extra.empty())) ||
        (argc == 1 && (ins.arg.empty() || !ins.extra.empty())) ||
        (argc == 2 && (ins.arg.empty() || ins.extra.empty())) ||
        out.code.size() >= MAX_INSTRUCTIONS) {
      error = "invalid instruction on line " + std::to_string(out.code.size() + 1);
      out.code.clear(); return false;
    }
    if ((ins.op == "FIELD" || ins.op == "ACTION" || ins.op == "ACTEQ" ||
         ins.op == "STATE" || ins.op == "BUMP") && !identifier(ins.arg)) {
      error = "invalid identifier: " + ins.arg; out.code.clear(); return false;
    }
    if (ins.op == "CONST" || ins.op == "BYTE" || ins.op == "PULSE" || ins.op == "JMP" ||
        ins.op == "JZ" || ins.op == "REPEATS" || ins.op == "MARKUS" ||
        ins.op == "SPACEUS" || ins.op == "BYTEVAL") {
      int64_t value;
      if (!number(ins.arg, value) ||
          ((ins.op == "BYTE" || ins.op == "PULSE" || ins.op == "JMP" || ins.op == "JZ") && value < 0) ||
          ((ins.op == "MARKUS" || ins.op == "SPACEUS") && (value < 50 || value > 1000000)) ||
          (ins.op == "BYTEVAL" && (value < 0 || value > 255))) {
        error = "invalid number in " + ins.op; out.code.clear(); return false;
      }
    }
    if (ins.op == "MATCHRAW" || ins.op == "MATCHBYTE") {
      int64_t index, value;
      if (!number(ins.arg, index) || index < 0 || !number(ins.extra, value) ||
          (ins.op == "MATCHRAW" && (index > 511 || value < -1000000 || value > 1000000)) ||
          (ins.op == "MATCHBYTE" && (index > 63 || value < 0 || value > 255))) {
        error = "invalid match instruction"; out.code.clear(); return false;
      }
    }
    out.code.push_back(std::move(ins));
  }
  if (out.code.empty() || out.code.back().op != (mode == Mode::RX ? "ACCEPT" : "SEND")) {
    error = "end the program with ACCEPT or SEND"; out.code.clear(); return false;
  }
  for (const auto &ins : out.code) {
    if (ins.op == "JMP" || ins.op == "JZ") {
      int64_t target;
      if (!number(ins.arg, target) || size_t(target) >= out.code.size()) {
        error = "jump target outside program"; out.code.clear(); return false;
      }
    }
  }
  error.clear(); return true;
}

bool receive(const Program &program, const std::string &protocol, const RFFrame &frame,
             DecodeResult &result, std::string &error) {
  result = DecodeResult{};
  result.protocol = protocol;
  result.device_type = "user";
  result.transmit_supported = false;
  return execute(program, Mode::RX, &frame, "", frame.channel, {}, &result, nullptr, error);
}

bool transmit(const Program &program, const std::string &action, uint8_t radio,
              const std::map<std::string, uint32_t> &state, TxFrame &frame, std::string &error) {
  frame = TxFrame{};
  return execute(program, Mode::TX, nullptr, action, radio, state, nullptr, &frame, error);
}

}  // namespace user_vm
}  // namespace rflink_universal
}  // namespace esphome
