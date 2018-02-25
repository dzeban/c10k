variable "aws_access_key" {}
variable "aws_secret_key" {}

variable "aws_region" {
  default = "eu-west-1"
}

variable "aws_vpc_cidr" {
  default = "172.16.10.0/24"
}

variable "aws_ami_fedora27" {
  type = "map"

  default = {
    "us-east-1"      = "ami-bcf84ec6"
    "us-west-1"      = "ami-18c7f878"
    "us-west-2"      = "ami-3594414d"
    "eu-west-1"      = "ami-78389b01"
    "eu-central-1"   = "ami-5f7cf830"
    "ap-southeast-1" = "ami-37286754"
    "ap-northeast-1" = "ami-0550fe63"
    "ap-southeast-2" = "ami-9d58b6ff"
    "sa-east-1"      = "ami-0048336c"
  }
}
