# Distributions

The kiOS "core" project is predominantly focused on compiling the base
kiOS system, including kiOS-specific code and third party required
binaries, and bundling this as a standalone EFI binary.

The kiOS project maintains [documentation](setup/index.md) on various
ways to install this EFI and setup the system, however it attempts to be
reasonably unopinionated and platform agnostic.

## Bundled Distributions

Clearly, for convenience, there are a few scenarios in which a complete
"out of the box" experience is more desirable (most notably for cloud
installations, or hobbiest bare-metal setups).

### AWS

The [kios-aws][kios-aws] project contains code relating to the specific
bootstrapping steps involved when running in AWS.

This project also builds and maintains a set of public AWS AMI Images in
each AWS region.

[kios-aws]: https://github.com/EmilyShepherd/kios-aws

### Raspberry Pi

!!! danger
    This project is not currently maintained - a _lot_ has changed in
    kiOS since this project was last supported, so there are likely to
    be a number of issues.

There is also some support for the raspberry pi 4. Currently, these
files live in the kiOS core project, however the aim is to move this out
to a separate project at some point.
