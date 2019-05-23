## High priority

- support of several SDK versions
- add an interface to add button support from externals Python scripts
- Nano X / Blue support
- check api level
- print call stack on app segfault
- console ux
- headless mode

## Low priority

- Don't allow apps to make Linux syscalls since Qemu will happily forward them
  to the host. Solution: launch Qemu using a seccomp profile.
- Add check when accessing app memory in the syscall implementation.
- MCU: add animated text support
- cleanup app loading
- Qt: circle

## Issues

- Pressing left and right buttons aren't reliable. Maybe add a third button?
