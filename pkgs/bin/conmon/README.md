
# Conmon

Conmon is the container monitor used by kiOS - a persistent process of
conmon is created per container.

## Glib

Conmon has a runtime dependency on glib, which has a dependency on
libpcre. At this time, these are built as shared libraries however this
may change in future (as normally kiOS prefers to build single-use
libraries into the binary).

## Conmon vs Conmon-rs

We intend to switch over to [conmon-rs][conmon-rs] at some point, if
possible. This has a few advantages:

- It is a pod monitor, rather than container monitor, which means less
exec calls and less long lived processes in the host system.
- It does not require glib, so we can drop that large library.

Work to switch over to this library has not completed yet due to a few
issues:

- Poor musl support for rustc (this either statically builds musl into
the binary, or fails during dynamic builds)
- Binary size - the finished binary is currently too big and
optimisation has not been completed on this yet
- Instability of the final result (and conmon-rs has not yet reached v1)

[conmon-rs]: https://github.com/containers/conmon-rs
