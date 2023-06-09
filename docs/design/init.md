# No Configurable Init

kiOS does not have a conventional or generalised init program -
instead the init process is a custom program specifically designed
for kiOS.

## What is "init"?

When a computer boots up, the first stage is for the kernel itself to
prepare the system. When it is ready, it needs to hand over control to
"user space": the programs that will run on the machine.

The Linux kernel does not know what you will be running (for example, on
a personal machine, you might be running a graphical window manager, on
a server, it may need to start up an ssh daemon. To get around this lack
of knowledge, the kernel always starts a known program: `init`. This
normally sits at a predefined location, although can be overridden.

The init program, which always has a process id of 1, is the first
program that always runs on the system, and it is normally responsible
for performing some further system setup, and then kicking off the other
daemons and programs that need to run.

There are a few common init systems - if you are using a Linux machine,
you are very likely to be using one of these:

- System V
- SystemD
- OpenRC
- runit
- Upstart

Each of these systems tends to have its own configuration language,
syntax or system.

## kiOS Init

Unlike other init systems, kiOS' init is relatively non-configurable.
While at first this seems counter-initiative, this does _not_ limit the
configurability of the kiOS system.

This is because of the kiOS philosophy that _everything should run in
containers_. As a result of this approach, the only thing that our init
system has to do, is start the container runtime, and kubelet. Once
these have started, Kubernetes is in charge of what runs on the system,
effectively making Kubernetes itself an extension of the init system.

## Comparison of kiOS init with conventional init

In the following two examples, we are running a system with the
following components:

- An init process (kiOS init vs conventional init)
- A DHCPD System daemon for IP addresses
- Kubelet
- An application "container A"

In these examples, the kernel is implied, and the container runtime,
which is a discrete component of any containerised system, has been
excluded and can be thought of as merged with "kubelet" for the purpose
of simplicity.

### Conventional Init

``` mermaid
sequenceDiagram
  autonumber
  init-->>init: General System Setup
  init-->>init: Read configuration to determine programs to run
  par Run Programs
    loop DHCP Daemon
      init->>dhcpcd: Start
      dhcpcd->>init: Exit
    end
    loop Kubelet
      init->>kubelet: Start
      kubelet->>init: Exit
    end
  end
  kubelet-->>kubelet: Read configuration to determin containers to run
  par Run Containers
    loop Container A
      kubelet->>container A: Start
      container A->>kubelet: Exit
    end
  end
```

### kiOS Init

``` mermaid
sequenceDiagram
  autonumber
  init-->>init: Specific System Setup
  init->>kubelet: Start
  kubelet-->>kubelet: Read configuration to determine containers to run
  par Run Containers
    loop DHCP Daemon
      kubelet->>dhcpcd: Start
      dhcpcd->>kubelet: Exit
    end
    loop Container A
      kubelet->>container A: Start
      container A->>kubelet: Exit
    end
  end
```

## Reasoning

There are two key tenants justifying this design decision:

- Security
- Efficiency

### Security

One of kiOS' core principles is that everything that can run inside of
containers should run inside of containers. Using an init system which is
able to be configured to run anything else, therefore, would not only be
unnecessary, but potentially open up a backdoor to the operating system.

Most init systems can be reconfigured to run arbitrary extra services, as
root in the host namespace, relatively easily - a container running as
root, which is often the default, with access to the host file system,
could potentially reconfigure a configurable init system.

kiOS does not include this backdoor as its init is hard-coded to only
support starting the container runtime, and the kubelet, with tight
controls over the command line flags passed to these services.

### Efficiency

You will notice that kubernetes has a generic runtime loop for
containers:

- Start the container
- Keep track of its status
- If the container exits unexpectedly, restart

This loop is _identical_ to the runtime loop required by most generic
init systems. Clearly, having a system using _two_ runtime loops is
relatively inefficient and, as kiOS is a Kubernetes-only operating
system, the Kubernetes runtime loop is a given, so it makes sense in
our case to remove the loop at the init level.
