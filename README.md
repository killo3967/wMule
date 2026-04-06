# wMule

<p align="center">
  <img src="wmule.png" alt="wMule logo" width="320">
</p>

**Modernized aMule port for Windows with updated architecture and current technologies**

## Overview

wMule is a modernized Windows-focused port of aMule.

The project aims to preserve the core functionality of the original application while updating its architecture, strengthening security, improving maintainability, and preparing the codebase for modern integrations, external control layers, and future UI evolution.

## Goals

- Modernize the legacy aMule codebase for current Windows development environments
- Improve security, robustness, and long-term maintainability
- Reduce architectural coupling between core logic and legacy UI layers
- Prepare the project for modern remote-control and integration scenarios
- Keep compatibility and incremental validation as first-class constraints

## Current Status

Completed work includes:

- **Phase 0 — Baseline and governance**
  - Reproducible baseline established
  - Canonical build and test workflow documented
  - Initial contribution and branch policy defined

- **Phase 1 — Critical security hardening**
  - Network packet parsing hardened
  - Kad/UDP guards improved
  - UPnP flow hardened
  - Defensive tests and fuzz-oriented cases added

- **Phase 2 — Paths, files and remote configuration**
  - Path normalization and traversal protections added
  - GUI, CLI and remote connectors aligned under the same path validation rules
  - External Connections credentials migrated to safer PBKDF2-based storage
  - UPnP support kept operational with clearer feedback and fallback behavior

- **Phase 3 — x64 robustness and memory correctness**
  - Pointer and integer truncation issues addressed
  - Binary layouts documented and validated
  - Serialization and size-related issues reviewed for x64 safety

- **Phase 4 — Safe concurrency**
  - Completed in the current project status

## Active Roadmap

### Remote control protocol evolution

A major active track is the conservative evolution of the legacy **EC v2** remote-control protocol toward a new **PBgRPC** layer based on **Protocol Buffers + gRPC**.

This work is intended to:

- preserve EC v2 during the transition
- audit the current protocol against the real codebase
- define a formal mapping from EC v2 to PBgRPC
- introduce a parallel modern control channel
- create **wmulecmdv2** as a separate validation client for the new protocol

The objective is not to break the legacy protocol prematurely, but to build a safer and more maintainable remote-control surface in parallel.

### Next major phases

- **Phase 5 — Async incremental migration**
  - Controlled migration of selected flows to AsyncSocket
  - Metrics, backpressure, timeouts and regression-safe rollout

- **Phase 6 — Architectural refactor**
  - Reduce coupling between core and GUI
  - Move toward clearer module boundaries
  - Prepare the core for future external interfaces and services

- **Phase 7 — Quality, tests and CI**
  - Expand automated coverage
  - Add Windows CI
  - Integrate static analysis and quality gates

- **Phase 8 — Future evolution**
  - REST/API options
  - stronger client/server separation
  - future GUI replacement
  - possible progressive migration of selected modules

## Technical Direction

wMule follows a staged modernization strategy:

- **security first**
- **stability before new features**
- **incremental changes with validation**
- **protocol compatibility where required**
- **clear evidence through build and test gates**

## Build Environment

Current baseline:

- **Platform:** Windows x64
- **Toolchain:** CMake + vcpkg + MSVC 2022
- **Primary binaries:** `wmule.exe`, `wmulecmd.exe`

## Project Principles

- Preserve working functionality while modernizing internals
- Avoid large uncontrolled rewrites
- Validate each phase with build and test evidence
- Prefer explicit contracts and observable behavior over assumptions
- Use the codebase itself as the source of truth for legacy behavior

## Planned Long-Term Direction

The long-term direction of wMule is to become a safer, cleaner and more extensible codebase that can support:

- a decoupled core
- modern remote-control interfaces
- external GUIs
- more maintainable integrations
- gradual technological evolution without sacrificing compatibility too early

## Status Note

wMule is an active modernization effort, not just a rebrand. The project is being evolved in phases, with completed security and x64-hardening work already in place and further architectural modernization planned.

## License

TBD

## Acknowledgements

wMule builds on the legacy and functionality lineage of aMule while pursuing a Windows-focused modernization path.
