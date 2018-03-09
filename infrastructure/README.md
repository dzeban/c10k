# c10k test rig

This is the c10k test rig consisting of server and multiple clients all spawned
in the cloud (only AWS for now). 

Infrastructure is created with Terraform and provisioned with Ansible.

To spawn it, apply Terraform configuration and then launch `site.yml` playbook.

    $ terraform apply
    $ ansible-playbook site.yml 

To make these command successful, Terraform and Ansible require valid cloud
provider configuration described hereafter.

## AWS setup

AWS requires credentials in the form of access key id and secret access key.
Both Terraform and Ansible may use shared credentials file ~/.aws/credentials,
so it has to be configured.

Nevertheless, you can overide this with env variables `AWS_ACCESS_KEY_ID` and
`AWS_SECRET_ACCESS_KEY`

