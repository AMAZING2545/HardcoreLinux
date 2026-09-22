# Security Policy

Hardcore Linux takes security seriously, even as a small, independently maintained project. This document explains which versions receive fixes and how to report a vulnerability responsibly.

## Supported versions

| Version | Supported | Notes |
|---|---|---|
| `main` | ✅ | Rolling; packages and tools only |
| 1.3.x | ✅ | Includes kernel and driver fixes |
| 1.2.x | ✅ | Default userland only |
| 1.1.x | ✅ | Default userland only |
| < 1.1.0 | ⚠️ Limited | OpenSSL backports only — everything else is end-of-life |

Versions marked "limited" or unsupported will **not** receive fixes outside of critical OpenSSL/TLS issues. If you're running an unsupported version, please upgrade before reporting — we may ask you to reproduce the issue on a supported release first.

## Reporting a vulnerability

**Please do not open a public GitHub issue for security vulnerabilities.** Publicly disclosing a bug before a fix is available puts every user of Hardcore Linux at risk.

Instead, report it privately using one of the following channels, in order of preference:

1. **GitHub Private Vulnerability Reporting** (preferred): go to the [Security tab](https://github.com/AMAZING2545/HardcoreLinux/security) of this repository and select **"Report a vulnerability"**. This opens a private advisory visible only to maintainers.
2. If private reporting is unavailable to you, contact the maintainer directly and request a secure channel before sharing any details.

### What to include

To help us triage quickly, please provide:

- **Affected version(s)** and how you determined the version.
- **Component affected** — e.g. the init system (`tools/init`), a package manager (`flashman`/`yspm`), a specific package build recipe, the kernel config, etc.
- **Steps to reproduce**, as precise and minimal as possible.
- **Impact** — what an attacker could actually achieve (privilege escalation, arbitrary code execution, information disclosure, denial of service, etc.) and any preconditions required (local access, malicious package, network exposure...).
- **Logs, PoC code, or screenshots**, if applicable.
- Whether the issue is already public or has been shared elsewhere.

### What to expect

| Stage | Target timeline |
|---|---|
| Acknowledgment of your report | Within 5 business days |
| Initial assessment (valid / not valid, severity) | Within 10 business days |
| Fix or mitigation, for confirmed issues | Best effort, prioritized by severity |
| Public disclosure | Coordinated with you, after a fix is available |

This is a small, community-maintained project without a dedicated security team, so timelines are best-effort rather than contractual. We will keep you updated on progress even if a fix takes longer than expected.

### Coordinated disclosure

We ask reporters to give us reasonable time to investigate and release a fix before any public disclosure or blog post. In return, we will:

- Credit you (if you want to be credited) in the fix's release notes or advisory.
- Keep you informed of progress throughout the process.
- Publish a GitHub Security Advisory once a fix is available, describing the issue and affected versions.

If a vulnerability is actively being exploited in the wild, please flag this explicitly — it changes our response priority.

## Scope

In scope:

- The init system, service manager (`initctl`), and boot scripts under `tools/`
- Package managers (`flashman`, `yspm`) and the package installation/verification flow
- Build recipes under `build/` and `build.native/` (e.g. a recipe that introduces a supply-chain risk)
- Kernel and Busybox configuration shipped in `configs/`
- Official prebuilt packages and images distributed from this repository

Out of scope:

- Vulnerabilities in upstream third-party software itself (report those to the upstream project — e.g. a bug in `wlroots` or `musl` belongs upstream, not here), unless Hardcore's packaging or configuration introduces or worsens the issue.
- Issues that require physical access to an already-compromised or unlocked machine.
- Missing security hardening that is a deliberate design trade-off of a minimal distro (please open a normal discussion/issue for these instead).

## Acknowledgments

We appreciate the work of anyone who takes the time to responsibly report a security issue. Reporters who wish to be credited will be listed in the relevant security advisory once the fix is published.
