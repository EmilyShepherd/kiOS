# Node Configuration

!!! info
    This guide covers setting up a kiOS node if you know the details of
    your kubernetes cluster at install time, and want to embed these
    settings statically into your install

## Information You'll Need

The following information is required for kiOS to function within a
cluster:

- [x] The URL of your kubernetes cluster api-server
- [x] The public certificate of your cluster api-server
- [x] Credentials for kubelets to authenticate with your cluster api-server
- [x] The desired hostname for the node

## Installation Steps

### Hostname

The node's hostname can be set by saving the desired hostname to
`/etc/hostname`. This is automatically read and updated during system
boot, or kubelet reboots.

### The Cluster CA

??? warning "Required"
    It _is_ possible to configure a kubelet which does not use a CA for
    authentication which would allow you to skip this step, however that
    is considered an advanced technique and therefore not mentioned in
    this guide.

It is a good idea to place a copy of your api-server's public root CA
certificate on your nodes, so that the kubelet can confirm that the
api-server is genuine when communicating with it.

For incoming requests (from the api-server) kubelet on kiOS is
configured by default to look for the certificate used to verify the
request at `/etc/kubernetes/pki/ca.crt`. This path can be changed,
however it is rarely necessary to do so, so it is recommended you place
a copy of your Cluster's Root CA here.

### Kubelet Authentication

kiOS' kubelet connects to the api-server using a standard kubeconfig
file, much like you would find on your own machine under
`~/.kube/config`.

This file should be created at `/etc/kubernetes/kubelet.conf`:

=== "Username & Password"

    ```yaml
    apiVersion: v1
    kind: Config
    clusters:
    - name: default
      cluster:
        server: https://kube-apiserver.example.net:6443
        # You can also use certificate-authority-data to embed the
        # base64 encoded certificate directly into this file
        certificate-authority: /etc/kubernetes/pki/ca.crt
    users:
    - name: default
      user:
        username: example-user
        password: example-password
    contexts:
    - name: default
    context:
        cluster: default
        user: default
    current-context: default
    ```

=== "Token"

    ```yaml
    apiVersion: v1
    kind: Config
    clusters:
    - name: default
      cluster:
        server: https://kube-apiserver.example.net:6443
        # You can also use certificate-authority-data to embed the
        # base64 encoded certificate directly into this file
        certificate-authority: /etc/kubernetes/pki/ca.crt
    users:
    - name: default
      user:
        # You can also use tokenFile: to refer to a file containing the
        # secret token
        token: secret-token
    contexts:
    - name: default
    context:
        cluster: default
        user: default
    current-context: default
    ```

=== "Signed Certificate"

    ```yaml
    apiVersion: v1
    kind: Config
    clusters:
    - name: default
      cluster:
        server: https://kube-apiserver.example.net:6443
        # You can also use certificate-authority-data to embed the
        # base64 encoded certificate directly into this file
        certificate-authority: /etc/kubernetes/pki/ca.crt
    users:
    - name: default
      user:
        # You can also use client-certificate-data and client-key-data
        # to embed the base64 encoded certificate / key directly into
        # this file.
        client-certificate: /etc/kubernetes/pki/client.crt
        client-key: /etc/kubernetes/pki/client.key
    contexts:
    - name: default
    context:
        cluster: default
        user: default
    current-context: default
    ```

### Bootstrap Authentication

!!! info "Optional"

Sometimes, you have temporary credentials, which can be used once to
initially register the node, but are then swapped out for cluster signed
certificates going forward.

In this case, you should create the kubeconfig file, in the same format
as above, but placed at `/etc/kubernetes/bootstrap-kubelet.conf`
instead.

!!! warning
    If you are using Bootstrap authentication you **must not** create a
    file at `/etc/kubernetes/kubelet.conf`, as this takes precedence.

The most common case for this setup is when using
[bootstrap tokens][bootstrap-token] however any of the standard
authentication methods (username/password, exec, signed certificate,
etc) will work in this file.

You can read more details about the Kubelet TLS Bootstrapping process in
the [Kubernetes Documentation][tls-bootstrap]

[tls-bootstrap]: https://kubernetes.io/docs/reference/access-authn-authz/kubelet-tls-bootstrapping/
[bootstrap-token]: https://kubernetes.io/docs/reference/access-authn-authz/bootstrap-tokens/
