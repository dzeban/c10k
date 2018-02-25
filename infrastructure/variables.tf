variable "ssh_key" {
  default     = "~/.ssh/id_rsa.pub"
  description = "Path to the SSH public key for accessing cloud instances. Used for creating AWS keypair."
}

variable "c10k_server_port" {
  default = 8282
}

variable "server_instance_type" {
  default = "t2.micro"
}
