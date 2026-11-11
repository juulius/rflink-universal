#include <cassert>
#include <iostream>
#include "library_reconciliation.h"
using namespace esphome::rflink_universal::library;

int main() {
  ItemIdentity builtin{ItemKind::DECODER, "decoder:ev1527", 4, "aaa", Source::BUILTIN, "", 0};
  ItemIdentity same{ItemKind::DECODER, "decoder:ev1527", 4, "aaa", Source::USER_IMPORTED, "", 0};
  auto r = compare(&builtin, &same);
  assert(r.state == ReconcileState::IDENTICAL_DUPLICATE);
  assert(r.recommended == Decision::USE_BUILTIN);
  assert(r.safe_auto_resolution && !r.needs_user_attention);

  ItemIdentity old{ItemKind::DECODER, "decoder:ev1527", 3, "bbb", Source::USER_IMPORTED, "", 0};
  r = compare(&builtin, &old);
  assert(r.state == ReconcileState::BUILTIN_NEWER);
  assert(r.recommended == Decision::USE_BUILTIN);
  assert(r.needs_user_attention && r.keep_current_active_until_confirmed);

  ItemIdentity fork{ItemKind::DEVICE_PROFILE, "profile:ev1527:4button", 2, "custom", Source::USER_LEARNED,
                    "profile:ev1527:4button", 1};
  ItemIdentity official{ItemKind::DEVICE_PROFILE, "profile:ev1527:4button", 3, "official", Source::BUILTIN, "", 0};
  r = compare(&official, &fork);
  assert(r.state == ReconcileState::USER_MODIFIED);
  assert(r.needs_user_attention && r.recommended == Decision::USE_BUILTIN);
  assert(keep_both_allowed(ItemKind::DEVICE_PROFILE));
  assert(!keep_both_allowed(ItemKind::DECODER));

  ItemIdentity newer_user{ItemKind::DECODER, "decoder:ev1527", 5, "future", Source::USER_IMPORTED, "", 0};
  r = compare(&builtin, &newer_user);
  assert(r.state == ReconcileState::USER_NEWER);
  assert(r.recommended == Decision::KEEP_USER);

  ItemIdentity equal_version_diff{ItemKind::DECODER, "decoder:ev1527", 4, "different", Source::USER_IMPORTED, "", 0};
  r = compare(&builtin, &equal_version_diff);
  assert(r.state == ReconcileState::CONFLICT);
  assert(r.needs_user_attention);

  std::cout << "Library reconciliation: duplicate/newer/fork/conflict safety rules PASS\n";
}
