# Restrict Syscalls with Seccomp

A System Call is the way processes request services or communicates with
the Linux Kernel. Syscalls can range from privileged operations - such
as loading kernel modules, killing processes, changing the filesystem
root - to relatively innocuous - such as opening or closing files,
executing a program, etc.

Seccomp - Linux's Secure Computing Mode - allows cluster administrators
to specifically limit which syscalls may be used by a container.

## Seccomp Overview

In Kubernetes, a seccomp "profile" is specified via a json file, with
the following format:

```json title="seccomp-profile.json"
{
  "defaultAction": /* seccomp action */
  "syscalls": [
    /* Any number of rules */
    {
      "names": [ /* list of syscall names */ ],
      "action": /* seccomp action */
    }
  ]
}
```

In kiOS, these should be saved to the `/etc/seccomp/` directory.

### Seccomp Actions

In this format, the seccomp action can be one of:

`SCMP_ACT_ALLOW`

:   Allow the syscall to go ahead as normal

`SCMP_ACT_LOG`

:   Log the syscall to the kernel's message buffer. This can normally be
    viewed by running `dmsg`.

`SCMP_ACT_ERRNO`

:   Disallow the syscall. If the process attempts to make the syscall,
    the kernel will kill the process.

## Using a Profile

Once you have created any desired profiles, you can assign your Pods to
use them via the `securityContext.seccompProfile` field:

```yaml
securityContext:
  seccompProfile:
    # The "Localhost" type indicates that we are going to refer to a
    # file on the local system
    type: Localhost

    # Any file created in the /etc/seccomp directory can be referenced in
    # this fashion
    localhostProfile: seccomp-profile.json
```

## Full Example

Create an "example" seccomp profile which allows file read / write and
related syscalls, logs module related syscalls, and blocks everything
else.

```json title="/etc/seccomp/example.json"
{
  /* Block everything not specified */
  "defaultAction": "SCMP_ACT_ERRNO",
  "syscalls": [
    /* Allow module related syscalls, but log them */
    {
      "action": "SCMP_ACT_LOG",
      "names": [
        "init_module",
        "delete_module",
        "finit_module"
      ]
    },
    /* Allow file opening, closing, reading and writing etc syscalls */
    {
      "action": "SCMP_ACT_ALLOW",
      "names": [
        "read",
        "write",
        "open",
        "close",
        "stat",
        "fstat",
        "lseek"
      ]
    }
  ]
}
```

Create a pod that uses this seccomp profile. If it attempts a syscall
not listed in the profile, it will be killed.

```yaml title="example-pod.yaml"
apiVersion: v1
kind: Pod
metadata:
  name: example
spec:
  containers:
  - name: example
    image: example.net/example:latest
    securityContext:
      seccompProfile:
        type: Localhost
        localhostProfile: example.json
```
