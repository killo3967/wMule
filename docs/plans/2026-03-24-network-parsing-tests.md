# Network Parsing Tests Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add deterministic unit tests and lightweight fuzz harnesses that cover UDP/Kad payload validation and UPnP XML/URL guards introduced in Fase 1.

**Architecture:** Keep all tests in the existing MuleUnit suite, introducing targeted fixtures per subsystem. Mock or stub network structures so tests never open sockets. Reuse shared helpers (`KadPacketGuards`, `UPnPUrlUtils`, `AssignIfLanUrl`) to avoid duplicate logic and gate new APIs behind `#ifdef TESTING` when needed.

**Tech Stack:** C++17 (MSVC), MuleUnit, CMake/ctest.

---

### Task 1: Client UDP Packet Tests

**Files:**
- Create: `unittests/tests/ClientUDPTest.cpp`
- Modify: `unittests/tests/CMakeLists.txt`
- Modify: `src/ClientUDPSocket.h` (add `friend` hooks under `#ifdef TESTING`)
- Modify: `src/ClientUDPSocket.cpp` (expose small helper guarded by `#ifdef TESTING`)

**Step 1: Write the failing test**

```cpp
TEST(ClientUDPTest, RejectsOverSizedKadDecompression)
{
    muleunit::ScopedWSA wsa; // helper to init winsock if needed
    TestableClientUDPSocket socket;
    std::vector<uint8> compressed(4 + MAX_KAD_UNCOMPRESSED_PACKET + 1, 0);
    // craft header: protocol=OP_KADEMLIAHEADER, opcode=0x00, read decompress flag
    ASSERT_TRUE(socket.InjectCompressedKadPacket(compressed.data(), compressed.size()));
    ASSERT_EQUALS(0u, socket.GetLastDeliveredBytes());
    ASSERT_TRUE(socket.LastLogContains("exceeds 131072"));
}
```

Add a second test ensuring `EncryptedDatagramSocket` returns zero-length on invalid padding.

**Step 2: Run tests to verify failure**

Run: `cmake --build build --config Debug && ctest -R ClientUDPTest -C Debug`

Expected: Test binary builds but fails because helpers don’t exist.

**Step 3: Implement minimal hooks**

- In `src/ClientUDPSocket.h` declare `class TestableClientUDPSocket : public CClientUDPSocket` inside `#ifdef TESTING` with helper methods to call the private decompress routine and inspect `m_lastPacketLen`.
- In `src/ClientUDPSocket.cpp` wrap the existing decompress path with `#ifdef TESTING` that forwards pointers/events to the test harness (e.g., store `m_lastDeliveredBytes`, capture log message strings in a static buffer).

**Step 4: Re-run focused tests**

Run: `ctest -R ClientUDPTest -C Debug`

Expected: PASS.

**Step 5: Commit**

```bash
git add src/ClientUDPSocket.* unittests/tests/ClientUDPTest.cpp unittests/tests/CMakeLists.txt
git commit -m "test: cover ClientUDP guards and encrypted drops"
```

---

### Task 2: Kademlia Handler Fuzz Harness

**Files:**
- Create: `unittests/tests/KadHandlerFuzzTest.cpp`
- Modify: `unittests/tests/CMakeLists.txt`
- Modify: `src/kademlia/net/KademliaUDPListener.h/.cpp` (add test-only entrypoint)

**Step 1: Write failing tests**

```cpp
TEST(KadHandlerFuzzTest, Process2SearchResponseRejectsUntracked)
{
    KadTestHarness harness;
    std::vector<uint8> packet = harness.MakeSearchResPacket(/*count=*/1, /*withTracking=*/false);
    ASSERT_RAISES(wxString, harness.Listener().Process2SearchResponse(
        packet.data(), packet.size(), harness.FakeIP(), harness.FakePort(), CKadUDPKey()));
}

TEST(KadHandlerFuzzTest, Process2PublishNotesRejectsTagFlood)
{
    KadTestHarness harness;
    std::vector<uint8> packet = harness.MakePublishNotesPacket(/*tagCount=*/MAX_KAD_TAGS + 1);
    ASSERT_RAISES(wxString, harness.Listener().Process2PublishNotesRequest(
        packet.data(), packet.size(), harness.FakeIP(), harness.FakePort(), CKadUDPKey()));
}
```

**Step 2: Run tests to confirm failure**

Command: `ctest -R KadHandlerFuzzTest -C Debug`

Expected: Build fails due to missing harness.

**Step 3: Implement harness**

- Under `#ifdef TESTING`, expose a lightweight `KadTestHarness` that constructs `CKademliaUDPListener` with mock dependencies (e.g., stub `theApp`, `CKademlia::GetPrefs` using minimal fake classes – reuse existing MuleUnit stubs from `ModernizationTest`).
- Provide helper methods to craft serialized packets with controllable lengths/tag counts.

**Step 4: Run tests**

`ctest -R KadHandlerFuzzTest -C Debug`

**Step 5: Commit**

```bash
git add src/kademlia/net/KademliaUDPListener.* unittests/tests/KadHandlerFuzzTest.cpp unittests/tests/CMakeLists.txt
git commit -m "test: fuzz Kad handlers for payload limits"
```

---

### Task 3: UPnP XML Safety Tests

**Files:**
- Create: `unittests/tests/UPnPXmlSafetyTest.cpp`
- Modify: `unittests/tests/CMakeLists.txt`
- Modify: `src/UPnPBase.h/.cpp` (introduce injectable logging sink under `#ifdef TESTING`)

**Step 1: Add failing tests**

```cpp
TEST(UPnPXmlSafetyTest, OnEventReceivedSkipsNullDoc)
{
    TestableUPnPControlPoint cp;
    cp.RegisterService("uuid:test", "http://127.0.0.1:1234/event");
    cp.OnEventReceived("uuid:test", 42, nullptr);
    ASSERT_TRUE(cp.LogContains("null document"));
}

TEST(UPnPXmlSafetyTest, SubscribeRejectsNonLanScpd)
{
    TestableUPnPControlPoint cp;
    TestableUPnPService svc(cp, "http://203.0.113.1/scpd.xml");
    ASSERT_FALSE(cp.TrySubscribe(svc));
    ASSERT_TRUE(cp.LogContains("missing LAN-safe"));
}
```

**Step 2: Run tests to see them fail**

`ctest -R UPnPXmlSafetyTest -C Debug`

**Step 3: Add test hooks**

- Guard new methods in `CUPnPControlPoint`/`CUPnPService` with `#ifdef TESTING` to expose minimal constructors that bypass libupnp registration and capture log strings in a ring buffer.
- Provide helper methods in the test fixture to inject fake services/devices without touching sockets.

**Step 4: Run tests**

`ctest -R UPnPXmlSafetyTest -C Debug`

**Step 5: Commit**

```bash
git add src/UPnPBase.* unittests/tests/UPnPXmlSafetyTest.cpp unittests/tests/CMakeLists.txt
git commit -m "test: cover UPnP XML/URL guards"
```

---

### Task 4: Consolidated Regression Run & Docs

**Files:**
- Modify: `docs/PLAN_MODERNIZACION_2.0.md`

**Step 1: Full build/test**

Run: `cmake --build build --config Debug && ctest --output-on-failure -C Debug`

**Step 2: Update plan**

- Mark the 1.3 parsing test items as `[x]` once merged.

**Step 3: Commit docs**

```bash
git add docs/PLAN_MODERNIZACION_2.0.md
git commit -m "docs: record network parsing regression tests"
```

---

Plan complete and saved to `docs/plans/2026-03-24-network-parsing-tests.md`. Two execution options:

1. **Subagent-Driven (this session)** – dispatch a fresh subagent per task with checkpoints between steps.
2. **Parallel Session (separate)** – open a dedicated session using superpowers:executing-plans for batched execution.

¿Con cuál avanzamos?
