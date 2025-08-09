# Contributing to CLKernel

Thank you for your interest in contributing to CLKernel! This document outlines the process for contributing to this revolutionary operating system kernel.

## 🚀 Quick Start

1. Fork the repository
2. Clone your fork: `git clone https://github.com/your-username/CLKernel.git`
3. Set up the development environment following [SETUP_WINDOWS.md](SETUP_WINDOWS.md)
4. Create a feature branch: `git checkout -b feature/your-feature-name`
5. Make your changes
6. Test your changes: `make test` or `build.bat`
7. Submit a pull request

## 🏗️ Development Environment

### Required Tools
- **NASM**: Assembly compiler for bootloader and kernel entry
- **GCC**: C compiler (preferably cross-compiler for i686-elf)
- **QEMU**: For testing the kernel in emulation
- **Git**: Version control

### Setup Instructions
Follow the detailed setup guide in [SETUP_WINDOWS.md](SETUP_WINDOWS.md) for Windows or use the Makefile for Unix-like systems.

## 📝 Coding Standards

### C Code Style
- Use 4-space indentation (no tabs)
- Follow K&R brace style
- Maximum line length: 100 characters
- Use descriptive variable and function names
- Add comprehensive comments for complex logic

### Assembly Code Style
- Use Intel syntax (NASM)
- Comment every non-obvious instruction
- Use meaningful labels
- Align data and code sections properly

### File Organization
```
kernel/
├── core/           # Core kernel functionality
├── modules/        # Hot-swappable modules
└── *.h            # Header files

boot/               # Bootloader code
src/                # Additional source files
docs/               # Documentation
```

## 🧪 Testing

### Before Submitting
1. **Build Test**: Ensure the kernel builds successfully
   ```bash
   make clean && make
   # or on Windows:
   build.bat clean && build.bat
   ```

2. **Boot Test**: Verify the kernel boots in QEMU
   ```bash
   make run
   # or on Windows:
   test_boot.bat
   ```

3. **Module Test**: Test hot-swappable modules if modified
4. **Documentation**: Update relevant documentation

### Test Categories
- **Unit Tests**: Individual component testing
- **Integration Tests**: Module interaction testing  
- **Boot Tests**: Kernel startup verification
- **Feature Tests**: Revolutionary feature validation

## 📋 Contribution Areas

### 🧠 AI Supervisor System
- Machine learning algorithms for fault detection
- Performance optimization models
- Predictive maintenance features

### 🔧 Module System
- New hot-swappable modules
- Module dependency management
- Runtime loading improvements

### 🔒 Security & Sandboxing
- Capability-based security enhancements
- Process isolation improvements
- Security audit tools

### 🎭 Actor Shell
- New interactive commands
- Scripting language features
- User interface improvements

### 📚 Documentation
- Code documentation
- Architecture guides
- Tutorial content

## 🐛 Bug Reports

Use the GitHub issue tracker to report bugs. Include:

1. **Environment**: OS, compiler versions, hardware
2. **Steps to Reproduce**: Clear, step-by-step instructions
3. **Expected Behavior**: What should happen
4. **Actual Behavior**: What actually happens
5. **Logs**: Relevant error messages or QEMU output

## 💡 Feature Requests

For new features:

1. Check existing issues to avoid duplicates
2. Describe the feature clearly
3. Explain the use case and benefits
4. Consider implementation complexity
5. Be prepared to contribute to the implementation

## 🔄 Pull Request Process

1. **Branch Naming**: Use descriptive names
   - `feature/ai-supervisor-enhancement`
   - `fix/bootloader-memory-issue`
   - `docs/architecture-update`

2. **Commit Messages**: Use clear, descriptive messages
   ```
   Add: New timer module for improved scheduling
   
   - Implements high-precision timing
   - Adds support for periodic callbacks
   - Integrates with AI supervisor for optimization
   ```

3. **Pull Request Description**:
   - Summarize changes clearly
   - Reference related issues
   - Include testing performed
   - Note any breaking changes

4. **Review Process**:
   - Code review by maintainers
   - Automated testing (when available)
   - Documentation review
   - Architecture compatibility check

## 🏆 Recognition

Contributors will be recognized in:
- `CONTRIBUTORS.md` file
- Release notes for significant contributions
- Project documentation credits

## 📞 Getting Help

- **Documentation**: Check existing docs first
- **Issues**: Search existing issues
- **Discussions**: Use GitHub Discussions for questions
- **Architecture**: Refer to [ARCHITECTURE.md](ARCHITECTURE.md)

## 📜 Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help maintain a welcoming environment
- Follow professional communication standards

## 🎯 Roadmap Alignment

Before major contributions, consider the project roadmap:

1. **Phase 1**: Core kernel stability
2. **Phase 2**: Revolutionary features completion
3. **Phase 3**: Performance optimization
4. **Phase 4**: Advanced AI integration

---

Thank you for contributing to the future of operating systems! 🚀
