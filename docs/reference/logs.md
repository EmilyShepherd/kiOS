# System Logs

kiOS is strictly designed to run as little as possible outside of
containers. There are, however, two services which logically _have_ to
run at the host-level, outside of containers:

- [Kubelet](../technical/files.md#kubelet), which is responsible for
  deciding which contains should be running
- [Cri-o](../technical/files.md#crio), which is responsible for actually
  running containers.

However, visibility of these services, and their logs, can still be
important for cluster operators. As a result, kiOS typically runs a
container for each of these services, so their logs can be viewed in a
standard way.

These containers will also terminate and restart if their respective
service restarts.

## Node Pod

The `kubelet` and `crio` containers are run inside a long running static
pod defined on each node, which is placed in the `kube-system`
namespace. You can confirm that each kiOS node has a corresponding
`node` pod by running:

```sh title="List of Nodes"
> kubectl get nodes
NAME                                             STATUS
ip-172-31-43-168.eu-central-1.compute.internal   Ready
```

```sh title="List of kube-system Pods"
> kubectl -n kube-system get pods
NAME                                                  READY   STATUS
node-ip-172-31-43-168.eu-central-1.compute.internal   2/2     Running
```

## Viewing Logs

In the above example, you can see that there are two containers running.
One of these is `kubelet` and the other is `crio`. The logs for these
can be viewed in the same way that you would for any other pod:

```sh title="Cri-o Logs"
> kubectl -n kube-systems logs node-ip-172-31-43-168.eu-central-1.compute.internal crio
time="2023-05-27 13:26:52.686526487Z" level=info msg="Starting CRI-O, version: 1.26.3"
...
```

```sh title="Kubelet Logs"
> kubectl -n kube-systems logs node-ip-172-31-43-168.eu-central-1.compute.internal kubelet
I0527 13:24:51.779259      85 server.go:413] "Kubelet version" kubeletVersion="v1.25.10"
...
```

## Scraping Logs

Many clusters have systems setup to scrape logs directly from each node
(for example using services like promtail). The logs for these services
are stored in the standard location for container logs, so existing log
scrapers continue to be compatible and able to scrape these.

!!! warning Caveat
    As the Node Pod is static, its name in the cluster appears as
    "node-[NODE NAME]". However, within the kiOS node itself, static
    pods are referred to _without_ the node name appended. As a result,
    log scrapers will find logs in `/var/logs/pods/node/` rather than
    `/var/logs/pods/node-[node name]/`
