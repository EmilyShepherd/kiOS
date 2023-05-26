# Container Image Signing

When kiOS needs to pull an image from a container registry, prior to
running a container inside a pod, it supports verifying that it has been
digitally signed by a trusted signer.

## Save Your GPG Key to disk

!!! warning inline end
    Remember that you only need to save the public component of your GPG
    key(s). Do not export your private keys!

In order to verify signatures, kiOS needs to have a copy of the public
keys of any key-pairs that will be used to sign images. There is no hard
and fast rule for where these should go, however a good rule of thumb
would be in a directory named: `/etc/gpg/`.

If you sign your images locally, you can export the public component of
them with the following command (where `key` is the key id, or email
address associated with the key:

```sh
gpg --export [key[ key[ key...]]] > public-key.gpg
```

## Set The Container Policy

The containers policy file (`/etc/containers/policy.json`) determines
the rules on how to trust and verify pulled images. By default, kiOS has
the rule set to `insecureAcceptAnything` which means that unsigned
images are allowed.

The easiest way to explain the format and options of this file, is to
give an example, showing some of the different formats and
configurations:

```json title="/etc/containers/policy.json"
{
  "transports": {
    "docker": {
      /* Anything in the docker.io/library namespace will match this rule */
      /* "insecureAcceptAnything" allows unsigned images */
      "docker.io/library": [{"type": "insecureAcceptAnything"}],

      /* Reject any image hosted on quay.io */
      "quay.io": [{"type": "reject"}],

      /* Images hosted on example.com will be accepted only if they are signed
        with a trusted key */
      "example.com": [{
        "type": "signedBy",
        "type": "GPGKeys",
        "keyPath": "/etc/gpg/public-key.gpg",
        "keyPaths": ["/etc/gpg/public-key-1.gpg", "/etc/gpg/public-key-2.gpg"],
        "keyData": "base64-encoded-keyring-data",
        "signedIdentity": {"type": "matchRespository"}
      }]
    }
  },

  /* Any images not matching the above will be caught by the default rule */
  /* This can intself be set to reject all, allow all or require signed images */
  "default": [{"type": "reject"]]
}
```

## Configure Look-aside Addresses

Often, the maintainers of a particular image are the ones who will sign
it, and that signature will be stored along with the image in the
registry. If this is the case, you do not need to do anything further -
signatures will be pulled along with the images for verification.

Sometimes, however, signatures are _not_ saved along with the images.
This can be for one of two reasons:

- You are using a container registry which does not support image
  signatures.
- You want to sign images that have been produced by someone else (for
  example, a cluster operator may want to sign certain images within the
  `docker.io/library` namespace, even if they didn't directly create /
  control them.

In this case, signatures can be stored separately to the images
themselves, in what is called a "look-aside server". To configure this,
we can create one or more files in the `/etc/containers/regigistries.d/`
directory. For example, to specify a look-aside server for images in the
`registry.example.net` registry, we might create the following file:

```yaml title="/etc/containers/registries.d/registry.example.net.yaml"
docker:
  registry.example.net:
    lookaside: https://lookaside.example.com
```

You can also specify a default look-aside server for all images:

```yaml title="/etc/containers/registries.d/default.yaml"
default-docker:
  lookaside: https://lookaside.example.org
```

!!! note
    The name of file is not particularly important, as long as it exists
    within the `/etc/containers/registries.d` directory. You can also
    specify more than one registry, or a registry + the default-docker
    block in a single file.
