FROM scratch

ADD /bin/kubeadm /bin/kubeadm

ENTRYPOINT ["/bin/kubeadm", "--config=/etc/kubeadm.yaml"]
CMD ["init"]
