# Contributing to XPhage

Welcome to the **XPhage** repository! XPhage is an open-source, high-performance programming language developed under **AeonCoreX Lab**. 

We highly appreciate community contributions, bug reports, and feedback. However, to maintain the architectural integrity, performance, and long-term vision of XPhage, we follow a strict governance model led by our **Main Lead Developer**.

---

## Governance & Architecture Control

* **Ownership:** XPhage is proprietary to and managed by **AeonCoreX Lab**.
* **The BDFL Model:** XPhage follows a Lead-Developer-Centric evolution model. All core architectural decisions, syntax specifications, and compiler designs are strictly determined by the **Main Lead Developer**.
* **Mandatory Approval:** No code changes—regardless of who reviews them—will be merged into the core ecosystem without the explicit review and **Final Approval** from the Main Lead Developer.

---

## Contribution Workflow

To ensure your development time is well-spent, please strictly follow these steps before writing any code:

### 1. Discuss Before You Code
Do not open a Pull Request (PR) for a major feature or syntax change out of nowhere. 
* Open an **Issue** or submit a **Feature Proposal (RFC)** discussing what you want to implement.
* Wait for the **Main Lead Developer** to review the idea and give you the green light ("Approved for Dev").

### 2. Branching & Code Standards
* Fork the repository and create a feature branch (`feature/your-feature-name` or `bugfix/issue-number`).
* Ensure your code adheres to low-level optimization standards (Rust/C++ safety patterns, strict performance constraints).
* Include comprehensive tests for any new compiler logic or standard library additions.

### 3. Submitting a Pull Request (PR)
When your PR is ready, fill out the provided template and ensure:
* Your branch is rebased with the latest `main` branch.
* All CI/CD build actions and automated tests pass successfully.

---

## The Code Review Process

1. **Initial Triaging:** A core maintainer may look at your PR to check for basic formatting, testing, and documentation correctness.
2. **Core Review:** If the PR touches the compiler codebase (`/compiler/`), it will automatically trigger a mandatory review routing to the **Main Lead Developer**.
3. **The Final Vet:** Even if approved by secondary maintainers, **only the Main Lead Developer holds the merge keys** for critical branches. 

We thank you for your cooperation in keeping XPhage optimized, secure, and robust!

— **The AeonCoreX Lab Team**
