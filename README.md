[![Release · CLKernel](https://img.shields.io/badge/Release-Download-blue?logo=github&style=for-the-badge)](https://github.com/Ryuu-neko/CLKernel/releases)

# CLKernel — x86 Kernel with AI Supervision & Hot Modules 🧠⚙️

![CLKernel banner](https://img.shields.io/badge/CLKernel-x86%20Kernel-brightgreen)  

A compact, hybrid x86 kernel that mixes microkernel principles with practical system services. It runs a supervised AI agent to monitor system behavior. It supports hot-swappable modules and an actor-model shell for isolating user tasks. Use it to experiment with modular OS design, secure sandboxing, and actor-based interfaces.

Badges
- Build status: ![build](https://img.shields.io/badge/build-passing-green)
- License: ![license](https://img.shields.io/badge/license-Apache%202.0-blue)
- Topics: actor-model, ai-supervision, kernel, microkernel, hot-swappable-modules

---

Table of contents
- Features
- Design and architecture
- Actor-based shell
- AI supervision
- Hot-swappable modules
- Sandboxing and security
- Quick start — download and run
- Development and building
- Testing and emulation (QEMU)
- Contributing
- Releases and downloads
- License

---

Features ✨
- Hybrid microkernel design. Keep core small. Push services to isolated modules.
- Actor-model shell. Run each command as an actor with message-passing I/O.
- AI supervision agent. Observe system metrics and enforce policies.
- Hot-swappable modules. Load, unload, and replace drivers and services at runtime.
- x86 focus. NASM and C sources tuned for modern x86-64 toolchains.
- Minimal bootloader. Chainloadable and QEMU-ready.
- Sandbox-first services. Use capability-style access control for modules.
- Tools and test images for QEMU and real hardware.

Why CLKernel
- Keep the kernel small. A small trusted core reduces attack surface.
- Isolate services. Each module runs without direct access to kernel internals.
- Observe the system. The AI agent provides real-time monitoring and pattern detection.
- Replace parts at runtime. Change drivers or services without rebooting.

Design and architecture 🏗️
- Kernel core: task scheduler, IPC, low-level memory management, interrupt dispatch.
- User services: file system, network stack, device drivers. Each runs as a module.
- Actor layer: a lightweight actor runtime lives in user space. Actors communicate via typed messages.
- Supervisor: an AI agent runs in a confined VM or process. It ingests metrics, logs, and IPC traffic and issues control decisions to modules through a secure channel.

Key terms
- Module: a loadable service or driver with a defined capability set.
- Actor: a single-threaded logical unit that processes messages.
- Supervisor: the AI process that watches and suggests or enforces actions.
- Capability: a fine-grained permission token for resources (I/O, memory ranges, devices).

Actor-based shell 🗣️
The shell follows the actor model:
- Each command runs as an actor.
- The shell sends messages for input, receives output via messages.
- Actors cannot directly access other actors’ memory.
- The shell exposes a small API: spawn, send, recv, close.

Benefits
- Predictable concurrency. No shared-memory races inside actors.
- Easy composition. Build pipelines by wiring actor message flows.
- Clear fault boundaries. An actor crash does not crash the shell.

AI supervision 🤖
The system uses an on-host or in-guest AI process that:
- Collects metrics: CPU, memory, IPC rates, syscalls, module heartbeats.
- Detects anomalies via heuristics and trained models.
- Issues soft commands (alerts) and hard commands (module restart, revocation).
- Uses a signed control channel and a policy engine to validate actions.

Modes
- Advisory mode: the AI logs observations and suggests actions.
- Enforced mode: the AI can instruct the kernel to isolate or replace modules.

Hot-swappable modules 🔌
Module lifecycle
- Install: a signed module loads and registers its capabilities.
- Activate: the kernel checks capabilities and authorizes access.
- Replace: stop the old module, migrate state, and enable the new module.
- Revoke: the kernel revokes capabilities and removes the module.

Implementation notes
- Modules run in isolated address spaces.
- A migration protocol attempts to preserve minimal state for live updates.
- Module signatures and a simple trust store prevent unauthorized replacements.

Sandboxing and security 🛡️
- Capability-based access control limits what a module may do.
- The kernel enforces memory separation and IPC validation.
- The supervisor runs in a sandbox and cannot access raw device memory.
- Logs and audit trails record supervisor decisions and module lifecycle events.

Quick start — download and run ▶️
Download the latest release artifact from the Releases page and run it locally. The release page contains prebuilt images and VM artifacts. Download the image and execute it in a VM.

- Visit and download from: https://github.com/Ryuu-neko/CLKernel/releases
- The release bundles include:
  - clkernel-x86-qemu.img (bootable image)
  - clkernel.iso (hybrid ISO)
  - tools and debug symbols
- After you download the image file from the Releases page, run it with your emulator or flash it to a device.

Example (QEMU)
1. Download the image from the releases page above.
2. Start QEMU:
qemu-system-x86_64 -machine accel=kvm -m 2048 -drive format=raw,file=clkernel-x86-qemu.img -serial stdio -display none
3. Interact via serial console.

If the release link ever changes or you cannot access the artifact, check the Releases section in this repo.

Development and building 🛠️
Prerequisites
- x86-64 GNU toolchain (gcc, ld, nasm)
- Make or Ninja
- QEMU for testing
- Python 3 for utilities

Core build steps
- Build the bootloader with NASM.
- Compile kernel C sources with -fno-omit-frame-pointer and tuned flags.
- Link the kernel using a custom linker script.
- Package a bootable raw image or ISO.

Typical commands
- Build: make all
- Run in QEMU: make qemu
- Clean: make clean

Directory layout (high level)
- /bootloader — NASM boot code and stage2
- /kernel — kernel C and assembly
- /modules — reference module sources
- /tools — build and packaging tools
- /docs — design docs and ABI specs

Testing and emulation (QEMU) 🖥️
- The repo includes QEMU scripts for headless tests, serial tests, and fuzz harnesses.
- Use the serial console for reproducible logs:
qemu-system-x86_64 -m 2G -serial mon:stdio -drive file=clkernel-x86-qemu.img,format=raw
- Use snapshot mode for fast iteration:
qemu-system-x86_64 -snapshot -m 1G -drive file=clkernel-x86-qemu.img,format=raw

Debugging
- GDB stub is available on a TCP port. Start QEMU with -S -gdb tcp::1234 and attach GDB.
- Kernel symbols live in the build artifacts for gdb.

Runtime internals
- Scheduler: priority-based with time slices.
- IPC: synchronous and asynchronous channels with message validation.
- Memory: a small virtual memory manager and a simple page-fault handler.
- Devices: a minimal PCI enumerator and a device capability broker.

Contributing 🤝
- Read the docs in docs/ before opening a PR.
- Follow the coding style in the repo.
- Write tests for new modules.
- Keep changes small and focused. Use branches named feature/<topic> or fix/<ticket>.
- Use signed commits for module updates when possible.

Issue workflow
- Open an issue with steps to reproduce and logs.
- Mark severity and attach a VM image or trace if helpful.
- For security issues, disclose privately per the repo SECURITY.md.

Code of conduct
- Be respectful in issues and PRs.
- Focus on technical details.
- Provide reproducible test cases.

Releases and downloads 📦
Find prebuilt images and tooling on the Releases page. Download the image and execute the VM or flash the artifact to your test device.

- Releases: https://github.com/Ryuu-neko/CLKernel/releases
- The Releases page contains images and checksums. Download the relevant artifact and run it per platform instructions.

Example release contents
- clkernel-<version>-qemu.img (raw image)
- clkernel-<version>.iso (hybrid ISO)
- checksums and detached signatures
- debug symbols as a separate archive

License 📄
This repository uses the Apache 2.0 license. See LICENSE for full text.

Images and resources
- Badges use shields.io.
- Project diagrams and internal images live in docs/diagrams.
- Logs and traces ship in the releases for reproducible runs.

Contact and links
- Issues: open an issue on the repository.
- Releases: https://github.com/Ryuu-neko/CLKernel/releases

Security audit and supply chain
- Modules must include a signed manifest.
- The build system can emit reproducible artifacts and checksums.
- Consider running the release image inside a VM or sandbox before use.

Example commands quick reference
- Build: make all
- Run in QEMU: qemu-system-x86_64 -m 2048 -drive format=raw,file=clkernel-x86-qemu.img -serial stdio
- Download release: visit the Releases page listed above and fetch the image; run the image in your emulator.

Keep the repo updated, test changes in QEMU, and sign modules before publishing. Check the Releases page for the latest build and artifacts.