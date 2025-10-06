# Compiler — Project

A compact DSL and compiler toolchain designed to simplify development of distributed systems by abstracting common patterns, enforcing safety, and producing optimized code for deployment.

## Key features
- Clear DSL primitives for communication, state, and fault-tolerance
- Static checks to catch common distributed-system errors early
- Optimizations for communication and resource usage
- Pluggable backends / code generation targets
- Lightweight simulator for rapid iteration

## Quick start
1. Clone the repository:
    git clone <repo-url>
2. Enter the project directory:
    cd Compiler/compiler/
3. Build and run:
    make all
    ./jade_compiler < input.jade

    OR

    make test (// for pre-test files)

    For Parse tree check the AST.tree file.

## Example DSL snippet
A minimal example to express a replicated counter:
```
replica Counter {
  state value: int = 0

  operation increment() {
     value = value + 1
     broadcast state_update(value)
  }

  on state_update(v: int) {
     value = max(value, v)
  }
}
```

## Contributing
- Read CONTRIBUTING.md for guidelines
- Open issues and PRs with tests and a clear description
- Keep changes focused and documented

## License
See LICENSE file in the repository for licensing details.