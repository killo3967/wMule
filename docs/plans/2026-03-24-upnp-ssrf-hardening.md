# UPnP SSRF Hardening Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Ensure wMule’s UPnP discovery/control stack only communicates with LAN devices, handles malformed XML safely, and exposes defensive logging for SSRF attempts.

**Architecture:** Reuse libupnp control-point flow but introduce shared URL/host guards plus XML-safe helpers inside `UPnPBase`. Skip any device/service/action whose resolved endpoints are not LAN-safe, and make event/subscription handlers resilient to null documents.

**Tech Stack:** C++17 (MSVC), libupnp, MuleUnit, CMake.

---

### Task 1: Shared URL Guard Helpers

**Files:**
- Create: `src/UPnPUrlUtils.h`
- Modify: `src/UPnPBase.cpp:78-170`
- Modify: `src/UPnPBase.h`
- Modify: `unittests/tests/CMakeLists.txt`
- Create: `unittests/tests/UPnPUrlUtilsTest.cpp`

**Step 1: Write the failing test**

```cpp
TEST(UPnPUrlUtilsTest, RejectsPublicOrUnsupportedUrls)
{
    ASSERT_TRUE(IsSafeLanUrl("http://192.168.1.1/a"));
    ASSERT_TRUE(IsSafeLanUrl("https://10.0.0.2"));
    ASSERT_FALSE(IsSafeLanUrl("ftp://192.168.1.1"));
    ASSERT_FALSE(IsSafeLanUrl("http://1.2.3.4"));
    ASSERT_FALSE(IsSafeLanUrl("http://example.com"));
}
```

**Step 2: Run test to verify it fails**

Run: `cmake --build build --config Debug && ctest -R UPnPUrlUtilsTest -C Debug`  
Expected: Test fails with undefined references to helper functions.

**Step 3: Write minimal implementation**

```cpp
// src/UPnPUrlUtils.h
bool IsSafeLanUrl(const std::string& url);
bool ParseUrl(const std::string& url, std::string& scheme, std::string& host, uint16_t& port);
```

Move the existing static helpers from `UPnPBase.cpp` into the new header (plus a matching `.cpp` if necessary), expose them for reuse.

**Step 4: Wire helpers into UPnPBase**

Include the new header inside `UPnPBase.cpp`/`.h` and replace the old anonymous-namespace functions with calls to the shared helpers.

**Step 5: Run tests to verify they pass**

Run: `cmake --build build --config Debug && ctest -R UPnPUrlUtilsTest -C Debug`  
Expected: New test passes along with the existing suite.

**Step 6: Commit**

```bash
git add src/UPnPUrlUtils.h src/UPnPBase.cpp src/UPnPBase.h unittests/tests/CMakeLists.txt unittests/tests/UPnPUrlUtilsTest.cpp
git commit -m "feat: share and test UPnP LAN URL guards"
```

---

### Task 2: Reject Unsafe Device/Service URLs

**Files:**
- Modify: `src/UPnPBase.cpp:540-950`
- Modify: `src/UPnPBase.h`
- Optional Test: `unittests/tests/UPnPUrlUtilsTest.cpp`

**Step 1: Add helper to log-and-clear unsafe endpoints**

```cpp
static bool EnsureLanUrlOrClear(const std::string& label, const std::string& resolved, std::string& out);
```

**Step 2: Update `CUPnPService` constructor**

- After each successful `UpnpResolveURL`, call `EnsureLanUrlOrClear` to drop SCPD/control/event URLs outside LAN and emit a warning.
- If any required URL is missing, mark the service as invalid (`m_absSCPDURL.clear()`) and skip registering it.

**Step 3: Update `CUPnPDevice` constructor**

- Validate `manufacturerURL`, `modelURL`, and `presentationURL` using the shared helper; reject presentation pages outside LAN before storing them.

**Step 4: Update `CUPnPControlPoint::AddRootDevice`**

- Ensure `URLBase` (both original & fixed) is LAN-only; if not, log and skip.

**Step 5: Run focused tests**

Run: `cmake --build build --config Debug && ctest -R UPnPUrlUtilsTest -C Debug`  
Expected: Helper tests still pass (these code paths aren’t unit-tested directly, so rely on build success).

**Step 6: Commit**

```bash
git add src/UPnPBase.cpp src/UPnPBase.h
git commit -m "feat: drop UPnP services with non-LAN endpoints"
```

---

### Task 3: Harden Action/Event Execution

**Files:**
- Modify: `src/UPnPBase.cpp:695-1490`
- Modify: `src/UPnPBase.h`

**Step 1: Guard action execution paths**

- In `CUPnPService::Execute`, `GetStateVariable`, and subscription helpers, bail out early (with logs) if `m_absControlURL` or `m_absSCPDURL` is empty.
- Wrap `UpnpActionInvoke` and `UpnpSubscribe` calls with checks for LAN-safe URLs.

**Step 2: Make XML parsing defensive**

- When iterating over `IXML_Element` nodes (service/device lists, events), verify pointers before dereferencing and log + skip when tags are missing.
- Ensure `CUPnPControlPoint::Callback` checks `location` and `URLBase` for LAN safety before downloading documents.

**Step 3: Harden event receive path**

- In `OnEventReceived`, ignore empty `changedVariables` documents and log anomalies instead of assuming valid XML.

**Step 4: Run regression tests**

Run: `cmake --build build --config Debug && ctest --output-on-failure -C Debug`

**Step 5: Commit**

```bash
git add src/UPnPBase.cpp src/UPnPBase.h
git commit -m "feat: harden UPnP action and event handling"
```

---

### Task 4: Update Plan + Document Validation

**Files:**
- Modify: `docs/PLAN_MODERNIZACION_2.0.md`

**Step 1: Reflect progress**

- Mark Bloque 1.2 sub-items as `[~]`/`[x]` accordingly and describe the LAN-only policy + new tests in the notes section.

**Step 2: Run full suite**

Run: `cmake --build build --config Debug && ctest --output-on-failure -C Debug`

**Step 3: Commit docs**

```bash
git add docs/PLAN_MODERNIZACION_2.0.md
git commit -m "docs: log UPnP hardening progress"
```

---

Plan complete and saved to `docs/plans/2026-03-24-upnp-ssrf-hardening.md`. Two execution options:

1. **Subagent-Driven (this session)** – I dispatch a fresh subagent per task with reviews between steps for tight feedback.
2. **Parallel Session (separate)** – Open a new session using superpowers:executing-plans for batched execution with checkpoints.

Which approach prefieres?
