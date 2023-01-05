# iptables

Iptables is used by kubelet for some legacy rules, and cri-o for host
port forwarding.

NB: We only build iptables legacy (not nftables) as kubelet does not
support this.

## Ongoing support

The usage of iptables is being phased out of kubelet, however it remains
required by cri-o. Cri-o does have support for nftables, and kiOS _may_
patch cri-o to an ipvs implementation of portmap. As a result this
package may be updated, or even removed, in future.
