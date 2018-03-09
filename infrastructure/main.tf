provider "aws" {
  region     = "${var.aws_region}"
}

resource "aws_key_pair" "aws_keypair" {
  key_name   = "terraform_test"
  public_key = "${file(var.ssh_key_public)}"
}

resource "aws_vpc" "vpc" {
  cidr_block = "${var.aws_vpc_cidr}"

  tags = {
    Name = "terraform_test_vpc"
  }
}

resource "aws_internet_gateway" "terraform_gw" {
  vpc_id = "${aws_vpc.vpc.id}"

  tags {
    Name = "Internet gateway for C10K"
  }
}

resource "aws_route_table" "route_table" {
  vpc_id = "${aws_vpc.vpc.id}"

  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = "${aws_internet_gateway.terraform_gw.id}"
  }

  tags {
    Name = "C10K route table"
  }
}

resource "aws_subnet" "subnet" {
  vpc_id     = "${aws_vpc.vpc.id}"
  cidr_block = "${aws_vpc.vpc.cidr_block}"

  # map_public_ip_on_launch = true
  tags = {
    Name = "terraform_test_subnet"
  }
}

resource "aws_route_table_association" "a" {
  subnet_id      = "${aws_subnet.subnet.id}"
  route_table_id = "${aws_route_table.route_table.id}"
}

resource "aws_security_group" "server_sg" {
  vpc_id = "${aws_vpc.vpc.id}"

  # SSH ingress access for provisioning
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
    description = "Allow SSH access for provisioning"
  }

  ingress {
    from_port   = "${var.c10k_server_port}"
    to_port     = "${var.c10k_server_port}"
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
    description = "Allow access to c10k servers"
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

resource "aws_instance" "server" {
  ami                         = "${var.aws_ami_fedora27[var.aws_region]}"
  instance_type               = "${var.server_instance_type}"
  count                       = 1
  subnet_id                   = "${aws_subnet.subnet.id}"
  vpc_security_group_ids      = ["${aws_security_group.server_sg.id}"]
  key_name                    = "${aws_key_pair.aws_keypair.key_name}"
  associate_public_ip_address = true

  tags {
    Name = "C10K Server"
  }

  provisioner "remote-exec" {
    # Install Python for Ansible
    inline = ["sudo dnf -y install python"]

    connection {
      type        = "ssh"
      user        = "fedora"
      private_key = "${file(var.ssh_key_private)}"
    }
  }

  provisioner "local-exec" {
    command = "ansible-playbook -u fedora -i '${self.public_ip},' --private-key ${var.ssh_key_private} -T 300 provision.yml" 
  }
}
