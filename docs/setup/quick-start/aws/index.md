# AWS Installation

kiOS for AWS has been setup to try to be a drop in replacement for
EKS nodes, so it connects to an API server in the same way that
"vanilla" EKS node images do: using AWS' instance roles.

If you have an existing EKS cluster, and would like to migrate your
nodes from the EKS-provided image to kiOS, see the
[EKS Migration Guide](migrate-from-eks.md).

If you are setting up a cluster from scratch, see the [AWS Cluster From Scratch](new-cluster.md)
guide.


## AMI IDs

`v1.25.0-alpha3` is available as a prebuilt AMI in the following
regions:

| Region         | AMI ID                |
| -------------- | --------------------- |
| eu-central-1   | ami-0c95db368938356d9 |
| ap-south-1     | ami-0ae610b9a099ab326 |
| eu-north-1     | ami-086a2f07e3a613f23 |
| eu-west-3      | ami-0352fb306b7b1bdae |
| eu-west-2      | ami-0d9e90f925f6efcb8 |
| eu-west-1      | ami-07152e399f3c2ecf4 |
| ap-northeast-3 | ami-01d71e7639818e91d |
| ap-northeast-2 | ami-04c032f008a7cd895 |
| ap-northeast-1 | ami-0d9e800ec72a16877 |
| ca-central-1   | ami-0d0c758fc5c2d20ca |
| sa-east-1      | ami-039875f19eec7e550 |
| ap-southeast-1 | ami-098960be091fb2a2e |
| ap-southeast-2 | ami-0122b6b53b63f50d5 |
| us-east-1      | ami-0b02881901dcd7366 |
| us-east-2      | ami-0ddc8d3c49870d0a3 |
| us-west-1      | ami-0096d677a951fcbf3 |
| us-west-2      | ami-05cdecc9b08f7df28 |
